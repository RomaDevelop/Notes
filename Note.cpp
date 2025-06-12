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
	str.append(name).append(" ").append(group).append("\n");
	str.append("notif, postpone, last updated: ").append(dtNotify.toString(Fields::dtFormat()));
	str.append(" ").append(dtPostpone.toString(Fields::dtFormat())).append(" ").append(dtLastUpdated.toString(Fields::dtFormatLastUpated()));
	str.append("\nContent: ").append(content.size() < 20 ? content : content.left(17) + "...");
	return str;
}

void Note::DialogMoveToGroup()
{
	QStringPairVector grDatas = DataBase::GroupsIdsAndNames();
	static QString createNew = "Create new";

	QStringList list {createNew};
	int prependedCount = list.size();

	int currentIndex = -1;
	for(uint i=0; i<grDatas.size(); i++)
	{
		auto &grData = grDatas[i];
		list.append("");

		if(DataBase::IsGroupLocalById(grData.first)) list.back() = grData.second + " (local)";
		else list.back() = grData.second + " (global)";

		if(grData.second == group)
		{
			list.back() += " (current)";
			currentIndex = i;
		}
	}
	auto res = MyQDialogs::ListDialog("Moving note to group", list);
	if(!res.accepted) return;

	res.index -= prependedCount;

	if(res.index == currentIndex) return;

	if(res.choosedText == createNew)
	{
		DialogCreateNewGroup();
		return;
	}

	if(res.index < 0) QMbError("unexpacted error res.index < 0");

	MoveToGroup(grDatas[res.index].second);
}

void Note::DialogEditCurrentGroup()
{
	QMbError("Unrealesed");
}

void Note::DialogCreateNewGroup()
{
	auto answ = MyQDialogs::CustomDialog("Group creation", "Choose group type to create",{"local","global"});
	if(answ == "local")
	{
		auto inpRes = MyQDialogs::InputLine("Group creation", "Input new local group name");
		if(!inpRes.accepted) return;
		if(inpRes.text.isEmpty()) { QMbError("Empty group name"); return; }

		auto resId = DataBase::TryCreateNewLocalGroup(inpRes.text);
		if(resId < 0) QMbInfo("Group " + inpRes.text + " created");
		else QMbError("TryCreateNewGroup in local base error, resId("+QSn(resId)+") >= 0");
	}
	else if (answ == "global")
	{
		if(!netClient->canNetwork)
		{
			QMbError("Server not connected, operation impossible");
			return;
		}

		auto inpRes = MyQDialogs::InputLine("Group creation", "Input new global group name");
		if(!inpRes.accepted) return;
		if(inpRes.text.isEmpty()) { QMbError("Empty group name"); return; }

		QString newGroupName = std::move(inpRes.text);

		auto answFoo = [newGroupName](QString &&answContent){
			auto idNewGroup = NetConstants::GetFromAnsw_try_create_group_IdGroup(answContent);
			if(idNewGroup < 0) QMbError("Group was not created, answer is " + QSn(idNewGroup) + "");
			else
			{
				auto resId = DataBase::TryCreateNewGlobalGroup(newGroupName, QSn(idNewGroup));
				if(resId == idNewGroup) QMbInfo("Group " + newGroupName + " created");
				else QMbError("TryCreateNewGroup in local base error, resId differs: " + QSn(resId));
			}
		};

		netClient->RequestToServerWithWait(netClient->socket, NetConstants::request_try_create_group(), newGroupName, std::move(answFoo));
	}
}



void Note::MoveToGroup(QString newGroupName)
{
	auto newGroupId = DataBase::GroupId(newGroupName);
	if(newGroupId.isEmpty()) { QMbError("ChangeGroup:: grId.isEmpty() for group " + newGroupName); return; }

	if(DataBase::IsGroupLocalByName(group) && DataBase::IsGroupLocalById(newGroupId))
	{
		MoveToGroupOnClient(newGroupId, newGroupName);
		return;
	}

	QMbError("MoveToGroup not local work need refactor");
	return;

	if(group == defaultGroupName2()) // перемещение из дефолтной группы. Заметка будет созаваться на сервере
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
		tmpNote.group = newGroupName;
		netClient->RequestToServerWithWait(netClient->socket, NetConstants::request_create_note_on_server(), tmpNote.ToStr_v1(), std::move(answFoo));
	}
	else // перемещение не из дефолтной, заметка на сервере уже существует
	{
		auto answFoo = [this, newGroupId, newGroupName](QString &&answContent){
			if(answContent == NetConstants::success())
			{
				MoveToGroupOnClient(newGroupId, newGroupName);
			}
			else QMbError("Can't move note to group, bad server answ");
		};

		dtLastUpdated = QDateTime::currentDateTime();
		auto request = NetConstants::MakeRequest_move_note_to_group(QSn(this->idOnServer), newGroupId, dtLastUpdated);
		netClient->RequestToServerWithWait(netClient->socket, NetConstants::request_move_note_to_group(), std::move(request), std::move(answFoo));
	}
}

void Note::MoveToGroupOnClient(const QString &newGroupId, const QString &newGroupName)
{
	if(!DataBase::MoveNoteToGroupOnClient(QSn(id), newGroupId, DtLastUpdatedStr()))
	{
		QMbError("DataBase::MoveNoteToGroup returned false; tryed move to " + newGroupName);
		return;
	}

	group = std::move(newGroupName);
	EmitCbs(cbsGroupChanged);
}

void Note::InitFromTmpNote(Note &note)
{
	name = std::move(note.name);
	activeNotify = note.activeNotify;
	dtNotify = std::move(note.dtNotify);
	dtPostpone = std::move(note.dtPostpone);
	group = note.group;
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
	group = DataBase::GroupName(row[Fields::idGroupIndInNotes]);
	id = row[Fields::idNoteInd].toInt();
	idOnServer = row[Fields::idNoteOnServerInd].toInt();
	content = std::move(row[Fields::contentInd]);
	dtLastUpdated = QDateTime::fromString(row[Fields::lastUpdatedInd], Fields::dtFormatLastUpated());
}

Note Note::CreateFromRecord(QStringList & record)
{
	Note note;
	note.InitFromRecord(record);
	return note;
}

void Note::UpdateThisNoteFromSQL()
{
	auto rec = DataBase::NoteByIdOnServer(QSn(idOnServer));
	if(rec.isEmpty()) { QMbError("UpdateThisNoteFromSQL"); return; }
	InitFromRecord(rec);
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
	for(uint i=0; i<cbsDTUpdated.size(); i++)
	{
		auto &cb = cbsDTUpdated[i];
		cb.cb(cb.handler);
	}
}

void Note::SaveNoteOnClient(const QString &reason)
{
	qdbg << "SaveNote for "+reason+" name:" + name;

	dtLastUpdated = QDateTime::currentDateTime();

	auto saveRes = DataBase::SaveNoteOnClient(this);
	if(!saveRes.isEmpty())
	{
		QMbError("Saving note error: " + saveRes);
		return;
	}

	if(!DataBase::IsGroupLocalByName(group))
	{
		NetClient::AnswerWorkerFunction answFoo = [](QString &&answContent){
			if(answContent != NetConstants::success())
				QMbWarning("Server can't save note, it saved local");
		};

		if(netClient->canNetwork)
			netClient->RequestToServerWithWait(netClient->socket, NetConstants::request_note_saved(), ToStr_v1(), answFoo);
		else QMbWarning("Server not connected, note saved local");
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

Note Note::FromStr_v1(const QString &text)
{
	auto fileParts = text.split(SaveKeyWods::endValue());
	if(fileParts.size() < 10) { QMbError("Wrong file content (size="+QSn(fileParts.size())+")"); return {}; }

	Note newNote;
	newNote.name = std::move(fileParts[1]);
	newNote.id = fileParts[2].toLongLong();
	newNote.idOnServer = fileParts[3].toLongLong();
	newNote.activeNotify = fileParts[4].toLongLong();
	newNote.dtNotify = QDateTime().fromString(fileParts[5], Fields::dtFormat());
	newNote.dtPostpone = QDateTime().fromString(fileParts[6], Fields::dtFormat());
	newNote.group = std::move(fileParts[7]);
	newNote.content = std::move(fileParts[8]);
	newNote.dtLastUpdated = QDateTime().fromString(fileParts[9], Fields::dtFormatLastUpated());
	return newNote;
}

QString Note::ToStr_v1() const
{
	QString noteText;
	noteText.append(SaveKeyWods::version()).append("1").append(SaveKeyWods::endValue());
	noteText.append(name).append(SaveKeyWods::endValue());
	noteText.append(QSn(id)).append(SaveKeyWods::endValue());
	noteText.append(QSn(idOnServer)).append(SaveKeyWods::endValue());
	noteText.append(QSn(activeNotify)).append(SaveKeyWods::endValue());
	noteText.append(dtNotify.toString(Fields::dtFormat())).append(SaveKeyWods::endValue());
	noteText.append(dtPostpone.toString(Fields::dtFormat())).append(SaveKeyWods::endValue());
	noteText.append(group).append(SaveKeyWods::endValue());
	noteText.append(content).append(SaveKeyWods::endValue());
	noteText.append(dtLastUpdated.toString(Fields::dtFormatLastUpated())).append(SaveKeyWods::endValue());
	return noteText;
}

std::vector<Note> Note::LoadNotes()
{
	LoadNotesFromFilesAndSaveInBd();

	auto notes = DataBase::NotesFromBD();
	return notes;
}

void Note::LoadNotesFromFilesAndSaveInBd()
{
	std::vector<Note> notes;
	QString loadStartDt = QDateTime::currentDateTime().toString(DateTimeFormatForFileName);
	QString notesSavesPath = MyQDifferent::PathToExe() + "/files/notes";
	QString notesBackupsPath = MyQDifferent::PathToExe() + "/files/notes_backups";
	auto files = QDir(notesSavesPath).entryList(QDir::Files,QDir::Name);
	for(int i=0; i<files.size(); i++)
	{
		QString thisOptionDt = QDateTime::currentDateTime().toString(DateTimeFormatForFileName_ms);
		QString filePathName = notesSavesPath + "/" + files[i];
		QDir().mkpath(notesBackupsPath + "/" + loadStartDt);
		if(!QFile::copy(filePathName, notesBackupsPath + "/" + loadStartDt + "/" + thisOptionDt + " " + files[i]))
			QMbError("creation backup note error for file  " + filePathName);

		auto fileContent = MyQFileDir::ReadFile1(filePathName);

		auto note_uptr = LoadNote(fileContent);
		if(note_uptr)
		{
			Note &noteRef = *note_uptr.get();
			notes.emplace_back(std::move(noteRef));

			if(notes.back().group != notes.back().defaultGroupName2())
			{
				QMbWarning("note->group != note->defaultGroupName()");
				notes.back().group = notes.back().defaultGroupName2();
			}

			DataBase::InsertNoteInClientDB(&notes.back());
			qdbg << notes.back().name + " readed from file and inserted in base, and file removed";
			QFile::remove(filePathName);
		}
		else QMbError("error loading note from file " + filePathName);
	}
	notes.clear();
}

int Note::GetVersion(const QString &text)
{
	QStringRef begin(&text,0, text.size() > 100 ? 100 : text.size());
	int index = begin.indexOf(SaveKeyWods::endValue());
	if(index == -1) return -1;
	QString v = text.mid(SaveKeyWods::version().size(), index - SaveKeyWods::version().size());
	return v.toInt();
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


