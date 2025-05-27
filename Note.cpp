#include "Note.h"

#include <QDebug>

#include "MyQShortings.h"
#include "MyCppDifferent.h"
#include "MyQFileDir.h"
#include "MyQDialogs.h"
#include "MyQSqlDatabase.h"

#include "DataBase.h"

QString Note::ToStrForLog()
{
	QString str;
	str.append(name).append(" (").append(QSn(index)).append(") ").append(group->name).append("\n");
	str.append(dtNotify.toString(DateTimeFormat)).append(" ").append(dtPostpone.toString(DateTimeFormat));
	str.append("\nContent: ").append(content.size() < 50 ? content : content.left(47) + "...");
	return str;
}

NotesGroup* Note::AddGroup(QString groupName)
{
	if(groupsMap.count(groupName) > 0)
	{
		QMbError("Group " + groupName + " already exists");
		return nullptr;
	}

	groups.push_back(std::make_shared<NotesGroup>());
	NotesGroup* newGrPtr = groups.back().get();
	newGrPtr->name = std::move(groupName);
	groupsMap[newGrPtr->name] = newGrPtr;

	return newGrPtr;
}

QStringList Note::GroupsNames()
{
	QStringList res;
	for(auto &node:groupsMap)
	{
		res += node.first;
	}
	return res;
}

void Note::DialogMoveToGroup()
{
	QStringList grNames = GroupsNames();
	static QString createNew = "Create new";
	grNames.prepend(createNew);
	QString currentGroupRow = group->name + " (current)";
	for(auto &grName:grNames) if(grName == group->name) grName = currentGroupRow;
	auto res = MyQDialogs::ListDialog("Moving note to group", grNames);
	if(!res.accepted) return;

	if(res.choosedText == currentGroupRow) return;

	if(res.choosedText == createNew)
	{
		auto newGroup = DialogCreateNewGroup();
		if(newGroup) ChangeGroup(newGroup);
		return;
	}

	if(auto it = groupsMap.find(res.choosedText); it == groupsMap.end())
	{
		QMbError("group "+res.choosedText+" not found");
	}
	else
	{
		ChangeGroup(it->second);
	}
}

void Note::DialogEditCurrentGroup()
{

}

NotesGroup* Note::DialogCreateNewGroup()
{
	if(!netClient->canNetwork) return nullptr;

	netClient->RequestToServer(NetConstants::request_group_names(), "", {[](){ netClient->Log("answ get"); }});

	auto inpRes = MyQDialogs::InputLine("Group creation", "Input new group name");
	if(!inpRes.accepted) return nullptr;

	NotesGroup* newGroup = AddGroup(inpRes.text);

	return newGroup;
}

void Note::ChangeGroup(QString groupName, bool createNewIfNeed)
{
	if(auto it = groupsMap.find(groupName); it != groupsMap.end())
		group = it->second;
	else
	{
		if(createNewIfNeed)
		{
			auto newGroup = AddGroup(std::move(groupName));
			if(!newGroup) { QMbError("Error group creation"); return; }
			else group = newGroup;
		}
		else { QMbError("Group was not set"); return; }
	}
	for(auto &cb:cbsGroupChanged) cb.cb(cb.handler);
}

void Note::ChangeGroup(NotesGroup *newGroup)
{
	group = newGroup;
	for(auto &cb:cbsGroupChanged) cb.cb(cb.handler);
}

void Note::InitFromTmpNote(Note &note)
{
	name = std::move(note.name);
	activeNotify = note.activeNotify;
	dtNotify = std::move(note.dtNotify);
	dtPostpone = std::move(note.dtPostpone);
	group = note.group;
	index = note.index;
	id = note.id;
	content = std::move(note.content);
}

void Note::InitFromRecord(QStringList &row)
{
	name = std::move(row[Fields::nameNoteInd]);
	activeNotify = row[Fields::activeNotifyInd].toInt();
	dtNotify = QDateTime::fromString(row[Fields::dtNotifyInd], Fields::dtFormat());
	dtPostpone = QDateTime::fromString(row[Fields::dtPostponeInd], Fields::dtFormat());
	//group = note.group;
	id = row[Fields::idNoteInd].toInt();
	content = std::move(row[Fields::contentInd]);
}

void Note::SetName(QString newName)
{
	name = std::move(newName);
	for(auto &cb:cbsNameUpdated) cb.cb(cb.handler);
}

void Note::SetContent(QString content)
{
	this->content = std::move(content);
	for(auto &cb:cbsContentUpdated) cb.cb(cb.handler);
}

bool Note::CmpDTs(const QDateTime &dtNotify, const QDateTime &dtPostpone)
{
	return this->dtNotify == dtNotify && this->dtPostpone == dtPostpone;
}

void Note::SetDT(QDateTime dtNotify, QDateTime dtPostpone)
{
	this->dtNotify = std::move(dtNotify);
	this->dtPostpone = std::move(dtPostpone);
	for(auto &cb:cbsDTUpdated) cb.cb(cb.handler);
}

void Note::SaveNote(const QString &reason)
{
	qdbg << "SaveNote for "+reason+" i:" + QSn(index) + " name:" + name;
	DataBase::SaveNote(this);
	return;

	QString noteText;
	noteText.append(SaveKeyWods::version()).append("1").append(SaveKeyWods::endValue());
	noteText.append(this->name).append(SaveKeyWods::endValue());
	noteText.append(QSn(this->activeNotify)).append(SaveKeyWods::endValue());
	noteText.append(this->dtNotify.toString(SaveKeyWods::dtFormat())).append(SaveKeyWods::endValue());
	noteText.append(this->dtPostpone.toString(SaveKeyWods::dtFormat())).append(SaveKeyWods::endValue());
	noteText.append(SaveKeyWods::group()).append(group->name).append(SaveKeyWods::endValue());
	noteText.append(this->content).append(SaveKeyWods::endValue());

	NetNoteSaved(noteText);
}

std::unique_ptr<Note> Note::LoadNote(const QString &text, const QString &fileFrom)
{
	if(!text.startsWith(SaveKeyWods::version())) // не содержит версию - самая старая
	{
		auto fileParts = text.split(SaveKeyWods::endValue(), QString::SkipEmptyParts);
		if(fileParts.size() != 5) { QMbError("Wrong file " + Note::notesSavesPath + "/" + fileFrom); return {}; }

		auto noteUptr = std::make_unique<Note>(Note(
							   std::move(fileParts[0]),
							   fileParts[1].toInt(),
							   QDateTime().fromString(fileParts[2], SaveKeyWods::dtFormat()),
							   QDateTime().fromString(fileParts[3], SaveKeyWods::dtFormat()),
							   std::move(fileParts[4])
							   ));
		return noteUptr;
	}
	else // содержит версию
	{
		int v = GetVersion(text);
		if(loadsFunctionsMap.empty()) InitLoadsFooMap();
		if(auto it = loadsFunctionsMap.find(v); it != loadsFunctionsMap.end())
		{
			auto &loadFunctoin = it->second;
			auto noteUptr = std::make_unique<Note>(loadFunctoin(text));
			return noteUptr;
		}
		else { QMbError("Not found load function for version "+QSn(v)+" in file " + Note::notesSavesPath + "/" + fileFrom); return {}; }
	}

	return {};
}

Note Note::LoadNote_v1(const QString &text)
{
	auto fileParts = text.split(SaveKeyWods::endValue());
	if(fileParts.size() < 7) { QMbError("Wrong file content (size="+QSn(fileParts.size())+")"); return {}; }

	Note newNote;
	newNote.name = std::move(fileParts[1]);
	newNote.activeNotify = fileParts[2].toUInt();
	newNote.dtNotify = QDateTime().fromString(fileParts[3], SaveKeyWods::dtFormat());
	newNote.dtPostpone = QDateTime().fromString(fileParts[4], SaveKeyWods::dtFormat());

	fileParts[5].remove(0, SaveKeyWods::group().size());
	newNote.ChangeGroup(std::move(fileParts[5]), true);

	newNote.content = std::move(fileParts[6]);
	return newNote;
}

std::vector<Note> Note::LoadNotes()
{
	MyQSqlDatabase::Init(clientBase, {});

	std::vector<Note> notes;
	QString loadStartDt = QDateTime::currentDateTime().toString(DateTimeFormatForFileName);
	auto files = QDir(Note::notesSavesPath).entryList(QDir::Files,QDir::Name);
	for(int i=0; i<files.size(); i++)
	{
		QString thisOptionDt = QDateTime::currentDateTime().toString(DateTimeFormatForFileName_ms);
		QString filePathName = Note::notesSavesPath + "/" + files[i];
		QDir().mkpath(Note::notesBackupsPath + "/" + loadStartDt);
		if(!QFile::copy(filePathName, Note::notesBackupsPath + "/" + loadStartDt + "/" + thisOptionDt + " " + files[i]))
			QMbError("creation backup note error for file  " + filePathName);

		auto fileContent = MyQFileDir::ReadFile1(filePathName);

		auto note_uptr = LoadNote(fileContent, filePathName);
		if(note_uptr)
		{
			Note &noteRef = *note_uptr.get();
			notes.emplace_back(std::move(noteRef));
			notes.back().index = i;

			DataBase::InsertNoteInDefaultGroup(&notes.back());
			qdbg << notes.back().name + " readed from file and inserted in base, and file removed";
			QFile::remove(filePathName);
		}
	}
	notes.clear();

	DataBase::BackupBase();
	notes = DataBase::NotesFromDefaultGroup();

	return notes;
}

int Note::GetVersion(const QString &text)
{
	QStringRef begin(&text,0, text.size() > 100 ? 100 : text.size());
	int index = begin.indexOf(SaveKeyWods::endValue());
	if(index == -1) return -1;
	QString v = text.mid(SaveKeyWods::version().size(), index - SaveKeyWods::version().size());
	return v.toInt();
}

QString Note::MakeNameFileToSaveNote()
{
	QString fileName = notesSavesPath;
	fileName += "/note";
	fileName += QSn(index).rightJustified(5,'0');
	fileName += "-";
	fileName += QString(name).replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");
	fileName += ".txt";
	return fileName;
}

void Note::ExecRemoveNoteWorker()
{
	if(removeNoteWorker) removeNoteWorker();
	else qdbg << "Note::Remove() execed, but removeWorker not valid";
}

bool Note::RemoveNoteSQL()
{
	return DataBase::RemoveNote(this);
}

bool Note::CheckAlarm(const QDateTime & dateToCompare)
{
	return dateToCompare >= dtPostpone;
}

void Note::ShowDialogFastActions(QWidget *widgetToShowUnder)
{
	if(0) CodeMarkers::to_do("сделать нормально извлечение текста, а не через костыль QTextEdit");
	QTextEdit te;
	te.setHtml(this->Content());

	auto actions = FastActions::Scan(te.toPlainText());

	if(!actions.actionsVals.isEmpty())
		MyQDialogs::MenuUnderWidget(widgetToShowUnder, actions.actionsVals, actions.GetVectFunctions());
	else MyQDialogs::MenuUnderWidget(widgetToShowUnder, { MyQDialogs::DisabledItem("Actions not found") });
}

void Note::NetNoteSaved(QString &text)
{
	if(netClient->canNetwork)
	{
		netClient->SendToServer(NetConstants::note_saved(), false);
		netClient->SendToServer(text, true);
	}
}

void Note::AddCBNameUpdated(std::function<void (void *)> aUpdatedCb, void *handler, int &localCbCounter)
{
	if(aUpdatedCb && handler)
	{
		auto &cbRef = cbsNameUpdated.emplace_back();
		cbRef.cb = aUpdatedCb;
		cbRef.handler = handler;
		localCbCounter++;
	}
	else qdbg << "ConnectNameUpdated invalid cb or handler";
}

void Note::AddCBContentUpdated(std::function<void (void *)> aUpdatedCb, void *handler, int &localCbCounter)
{
	if(aUpdatedCb && handler)
	{
		auto &cbRef = cbsContentUpdated.emplace_back();
		cbRef.cb = aUpdatedCb;
		cbRef.handler = handler;
		localCbCounter++;
	}
	else qdbg << "ConnectContentUpdated invalid cb or handler";
}

void Note::AddCBDTUpdated(std::function<void (void *handler)> aUpdatedCb, void *handler, int &localCbCounter)
{
	if(aUpdatedCb && handler)
	{
		auto &cbRef = cbsDTUpdated.emplace_back();
		cbRef.cb = aUpdatedCb;
		cbRef.handler = handler;
		localCbCounter++;
	}
	else qdbg << "ConnectDTUpdated invalid cb or handler";
}

void Note::AddCBGroupChanged(std::function<void (void *)> aUpdatedCb, void *handler, int &localCbCounter)
{
	if(aUpdatedCb && handler)
	{
		auto &cbRef = cbsGroupChanged.emplace_back();
		cbRef.cb = aUpdatedCb;
		cbRef.handler = handler;
		localCbCounter++;
	}
	else qdbg << "SetCBGroupChanged invalid cb or handler";
}

void Note::RemoveCbs(void * handler, int removedCountShouldBe)
{
	int countRemoved = 0;
	for(uint i=0; i<cbsNameUpdated.size(); i++)
		if(cbsNameUpdated[i].handler == handler)
		{
			cbsNameUpdated.erase(cbsNameUpdated.begin()+i);
			countRemoved++;
		}
	for(uint i=0; i<cbsContentUpdated.size(); i++)
		if(cbsContentUpdated[i].handler == handler)
		{
			cbsContentUpdated.erase(cbsContentUpdated.begin()+i);
			countRemoved++;
		}
	for(uint i=0; i<cbsDTUpdated.size(); i++)
		if(cbsDTUpdated[i].handler == handler)
		{
			cbsDTUpdated.erase(cbsDTUpdated.begin()+i);
			countRemoved++;
		}
	for(uint i=0; i<cbsGroupChanged.size(); i++)
		if(cbsGroupChanged[i].handler == handler)
		{
			cbsGroupChanged.erase(cbsGroupChanged.begin()+i);
			countRemoved++;
		}
	if(removedCountShouldBe != countRemoved)
		QMbError("removedCountShouldBe != countRemoved for note " + name);
}


