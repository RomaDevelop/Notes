#include "Note.h"

#include <QDebug>
#include <QProgressDialog>
#include <QPointer>

#include "MyQShortings.h"
#include "MyCppDifferent.h"
#include "MyQFileDir.h"
#include "MyQDialogs.h"
#include "MyQSqlDatabase.h"

#include "InputBlocker.h"

#include "DataBase.h"

QString Note::ToStrForLog()
{
	QString str;
	str.append(name).append(" (").append(QSn(index)).append(") ").append(group).append("\n");
	str.append("notif, postpone, last updated: ").append(dtNotify.toString(Fields::dtFormat()));
	str.append(" ").append(dtPostpone.toString(Fields::dtFormat())).append(" ").append(dtLastUpdated.toString(Fields::dtFormatLastUpated()));
	str.append("\nContent: ").append(content.size() < 50 ? content : content.left(47) + "...");
	return str;
}

void Note::DialogMoveToGroup()
{
	QStringList grNames = DataBase::GroupsNames();
	static QString createNew = "Create new";
	grNames.prepend(createNew);
	QString currentGroupRow = group + " (current)";
	for(auto &grName:grNames) if(grName == group) grName = currentGroupRow;
	auto res = MyQDialogs::ListDialog("Moving note to group", grNames);
	if(!res.accepted) return;

	if(res.choosedText == currentGroupRow) return;

	if(res.choosedText == createNew)
	{
		DialogCreateNewGroup();
		return;
	}

	MoveToGroup(res.choosedText);
}

void Note::DialogEditCurrentGroup()
{

}

void Note::DialogCreateNewGroup()
{
	if(!netClient->canNetwork) return;

	auto inpRes = MyQDialogs::InputLine("Group creation", "Input new group name");
	if(!inpRes.accepted) return;
	if(inpRes.text.isEmpty()) { QMbError("Empty group name"); return; }

	QString newGroupName = std::move(inpRes.text);

	auto answFoo = [newGroupName](QString &&answContent){
		auto id = NetConstants::GetIdGroupFromAnsw_try_create_group(answContent);
		if(id < 0) QMbError("Group was not created, answer is " + QSn(id) + "");
		else
		{
			auto resId = DataBase::TryCreateNewGroup(newGroupName, QSn(id));
			if(resId == id) QMbInfo("Group " + newGroupName + " created");
			else QMbError("TryCreateNewGroup in local base error, resId differs: " + QSn(resId));
		}
	};

	netClient->RequestToServerWithWait(NetConstants::request_try_create_group(), newGroupName, std::move(answFoo));
}

void Note::MoveToGroup(QString newGroupName)
{
	auto newGroupId = DataBase::GroupId(newGroupName);
	if(newGroupId.isEmpty()) { QMbError("ChangeGroup:: grId.isEmpty() for group " + newGroupName); return; }

	if(group == defaultGroupName()) // перемещение из дефолтной группы. Заметка будет созаваться на сервере
	{
		auto answFoo = [this, newGroupId, newGroupName](QString &&answContent){
			if(0) CodeMarkers::to_do("if note will be removed before answ get will be trouble");

			auto idOnServer = NetConstants::GetFromAnsw_create_note_on_server_IdNoteOnServer(answContent);
			if(idOnServer < 0)
			{
				netClient->Error("bad answ to create note on server " + answContent);
				QMbError("Can't move note to group, bad server answ");
				return;
			}

			if(!DataBase::SetNoteFieldIdOnServer_OnClient(QSn(id), QSn(idOnServer)))
			{
				QMbError("DataBase::SetNoteIdOnServer returned false; tryed set " + QSn(idOnServer));
				return;
			}
			this->idOnServer = idOnServer;

			if(!DataBase::MoveNoteToGroupOnClient(QSn(id), newGroupId, DtLastUpdatedStr()))
			{
				QMbError("DataBase::MoveNoteToGroup returned false; tryed move to " + newGroupName);
				return;
			}

			group = std::move(newGroupName);
			EmitCbs(cbsGroupChanged);
		};

		this->dtLastUpdated = QDateTime::currentDateTime();
		Note tmpNote(*this);
		tmpNote.group = std::move(newGroupName);
		netClient->RequestToServerWithWait(NetConstants::request_create_note_on_server(), tmpNote.ToStrForNetSend(), std::move(answFoo));
	}
	else // перемещение не из дефолтной, заметка на сервере уже существует
	{
		auto answFoo = [this, newGroupId, newGroupName](QString &&answContent){
			if(answContent == NetConstants::success())
			{
				if(!DataBase::MoveNoteToGroupOnClient(QSn(id), newGroupId, DtLastUpdatedStr()))
				{
					QMbError("DataBase::MoveNoteToGroup returned false; tryed move to " + newGroupName);
					return;
				}

				group = std::move(newGroupName);
				EmitCbs(cbsGroupChanged);
			}
			else QMbError("Can't move note to group, bad server answ");
		};

		dtLastUpdated = QDateTime::currentDateTime();
		auto request = NetConstants::MakeRequest_move_note_to_group(QSn(this->idOnServer), newGroupId, dtLastUpdated);
		netClient->RequestToServerWithWait(NetConstants::request_move_note_to_group(), std::move(request), std::move(answFoo));
	}
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
	idOnServer = note.idOnServer;
	content = std::move(note.content);
	dtLastUpdated = std::move(note.dtLastUpdated);
}

void Note::InitFromRecord(QStringList &row)
{
	name = std::move(row[Fields::nameNoteInd]);
	activeNotify = row[Fields::activeNotifyInd].toInt();
	dtNotify = QDateTime::fromString(row[Fields::dtNotifyInd], Fields::dtFormat());
	dtPostpone = QDateTime::fromString(row[Fields::dtPostponeInd], Fields::dtFormat());
	group = DataBase::GroupName(row[Fields::idGroupInd]);
	id = row[Fields::idNoteInd].toInt();
	idOnServer = row[Fields::idNoteOnServerInd].toInt();
	content = std::move(row[Fields::contentInd]);
	dtLastUpdated = QDateTime::fromString(row[Fields::lastUpdatedInd], Fields::dtFormatLastUpated());
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

void Note::SaveNoteOnClient(const QString &reason)
{
	qdbg << "SaveNote for "+reason+" i:" + QSn(index) + " name:" + name;

	if(group == defaultGroupName()) DataBase::SaveNoteOnClient(this);
	else
	{
		if(!netClient->canNetwork) { QMbError("connection to server is lost, can't save this note"); return; }
		else
		{
			// удалить ноут айди
			// формировать и обрабатыать запрос на сохранение
			// а если не получилось - нужно откатывать изменения, мрак
			NetConstants::request_save_note();
			netClient->SendToServer(NetConstants::note_saved(), false);
			netClient->SendToServer(ToStrForNetSend(), true);
			netClient->Log("NetNoteSaved send");
		}
	}
}

std::unique_ptr<Note> Note::LoadNote(const QString &text)
{
	if(!text.startsWith(SaveKeyWods::version())) // не содержит версию - самая старая
	{
		auto fileParts = text.split(SaveKeyWods::endValue(), QString::SkipEmptyParts);
		if(fileParts.size() != 5) { qdbg << "Wrong file to load note"; return {}; }

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
			if(!noteUptr) qdbg << "loadFunctoin from loadsFunctionsMap returned nullptr";
			return noteUptr;
		}
		else { qdbg << "Not found load function for version "+QSn(v); return {}; }
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
	newNote.dtNotify = QDateTime().fromString(fileParts[3], Fields::dtFormat());
	newNote.dtPostpone = QDateTime().fromString(fileParts[4], Fields::dtFormat());
	newNote.group = std::move(fileParts[5]);
	newNote.group.remove(0, SaveKeyWods::group().size());

	newNote.content = std::move(fileParts[6]);
	if(fileParts.size() >= 8) newNote.dtLastUpdated = QDateTime().fromString(fileParts[7], Fields::dtFormatLastUpated());
	return newNote;
}

std::vector<Note> Note::LoadNotesFromFiles()
{
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

		auto note_uptr = LoadNote(fileContent);
		if(note_uptr)
		{
			Note &noteRef = *note_uptr.get();
			notes.emplace_back(std::move(noteRef));
			notes.back().index = i;

			if(notes.back().group != notes.back().defaultGroupName())
			{
				QMbWarning("note->group != note->defaultGroupName()");
				notes.back().group = notes.back().defaultGroupName();
			}

			DataBase::InsertNoteInClientDB(&notes.back());
			qdbg << notes.back().name + " readed from file and inserted in base, and file removed";
			QFile::remove(filePathName);
		}
		else QMbError("error loading note from file " + filePathName);
	}
	notes.clear();

	DataBase::BackupBase();
	notes = DataBase::NotesFromBD();

	return notes;
}

QString Note::ToStrForNetSend()
{
	QString noteText;
	noteText.append(SaveKeyWods::version()).append("1").append(SaveKeyWods::endValue());
	noteText.append(this->name).append(SaveKeyWods::endValue());
	noteText.append(QSn(this->activeNotify)).append(SaveKeyWods::endValue());
	noteText.append(this->dtNotify.toString(Fields::dtFormat())).append(SaveKeyWods::endValue());
	noteText.append(this->dtPostpone.toString(Fields::dtFormat())).append(SaveKeyWods::endValue());
	noteText.append(SaveKeyWods::group()).append(group).append(SaveKeyWods::endValue());
	noteText.append(this->content).append(SaveKeyWods::endValue());
	noteText.append(this->dtLastUpdated.toString(Fields::dtFormatLastUpated())).append(SaveKeyWods::endValue());
	return noteText;
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
	{
		qdbg << "RemoveCbs: removedCountShouldBe != countRemoved ("+QSn(removedCountShouldBe)
					+" != "+QSn(countRemoved)+") for note " + name;
		QMbError("RemoveCbs: removedCountShouldBe != countRemoved ("+QSn(removedCountShouldBe)
					+" != "+QSn(countRemoved)+") for note " + name);
	}
}


