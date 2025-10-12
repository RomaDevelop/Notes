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

Note::Note(QString name_, bool activeNotify_, QDateTime dtNotify_, QDateTime dtPostpone_, QString content_):
	activeNotify{activeNotify_},
	name {std::move(name_)},
	content {std::move(content_)},
	dtNotify {std::move(dtNotify_)},
	dtPostpone {std::move(dtPostpone_)}
{}



QString Note::ToStrForLog()
{
	QString str;
	str.append(name).append(" ").append(groupName).append("\n");
	str.append("notif, postpone, last updated: ").append(dtNotify.toString(Fields::dtFormat()));
	str.append(" ").append(dtPostpone.toString(Fields::dtFormat())).append(" ").append(dtLastUpdated.toString(Fields::dtFormatLastUpdated()));
	str.append("\nContent: ").append(content.size() < 20 ? content : content.left(17) + "...");
	return str;
}

void Note::DialogRenameNote()
{
	auto answ = MyQDialogs::InputLineExt("Renaming note", "Input new note name", name);
	if(answ.button == MyQDialogs::Accept() && answ.text != name)
	{
		if(answ.text.isEmpty()) { QMbError("Name can't be empty"); return; }
		SetName(answ.text);
	}
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

		if(grData.second == groupName)
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

	if(res.index < 0) Error("unexpacted error res.index < 0");

	MoveToGroup(grDatas[res.index].second);
}

void Note::DialogEditCurrentGroup()
{
	Error("Unrealesed");
}

void Note::DialogCreateNewGroup()
{
	auto answ = MyQDialogs::CustomDialog("Group creation", "Choose group type to create",{"local","global"});
	if(answ == "local")
	{
		auto inpRes = MyQDialogs::InputLine("Group creation", "Input new local group name");
		if(!inpRes.accepted) return;
		if(inpRes.text.isEmpty()) { Error("Empty group name"); return; }

		auto resId = DataBase::TryCreateNewLocalGroup(inpRes.text);
		if(resId < 0) QMbInfo("Group " + inpRes.text + " created");
		else Error("TryCreateNewGroup in local base error, resId("+QSn(resId)+") >= 0");
	}
	else if (answ == "global")
	{
		if(netClient->sessionId <= 0)
		{
			Error("Server not connected, operation impossible");
			return;
		}

		auto inpRes = MyQDialogs::InputLine("Group creation", "Input new global group name");
		if(!inpRes.accepted) return;
		if(inpRes.text.isEmpty()) { Error("Empty group name"); return; }

		QString newGroupName = std::move(inpRes.text);

		auto answFoo = [newGroupName](QString &&answContent){
			auto idNewGroup = NetConstants::GetFromAnsw_try_create_group_IdGroup(answContent);
			if(idNewGroup < 0) Error("Group was not created, answer is " + QSn(idNewGroup) + "");
			else
			{
				auto resId = DataBase::TryCreateNewGlobalGroup(newGroupName, QSn(idNewGroup));
				if(resId == idNewGroup) Error("Group " + newGroupName + " created");
				else Error("TryCreateNewGroup in local base error, resId differs: " + QSn(resId));
			}
		};

		netClient->RequestToServerWithWait(NetConstants::request_try_create_group(), newGroupName, std::move(answFoo));
	}
}

void Note::MoveToGroup(QString newGroupName)
{
	auto newGroupId = DataBase::GroupId(newGroupName);
	if(newGroupId.isEmpty()) { Error("ChangeGroup:: grId.isEmpty() for group " + newGroupName); return; }

	bool currentGroupIsLocal = DataBase::IsGroupLocalByName(groupName);
	bool newGroupIsLocal = DataBase::IsGroupLocalById(newGroupId);

	auto newDtUpdated = QDateTime::currentDateTime();

	// перемещение из локальной в локальную
	if(currentGroupIsLocal && newGroupIsLocal)
	{
		UpdateNote_group(newGroupId, newGroupName, newDtUpdated);
	}
	// перемещение из локальной в сетевую - на сервере будет создана новая
	else if(currentGroupIsLocal && !newGroupIsLocal)
	{
		auto answFoo = [this, newGroupId, newGroupName, newDtUpdated](QString &&answContent){
			if(0) CodeMarkers::to_do("if note will be removed before answ get it will be trouble");

			auto idNoteFromServer = NetConstants::GetFromAnsw_create_note_on_server_IdNoteOnServer(answContent);
			if(idNoteFromServer < 0)
			{
				netClient->Error("bad answ to create note on server " + answContent);
				Error("Can't move note to group, bad server answ");
				return;
			}

			UpdateNote_id_group(idNoteFromServer, newGroupId, newGroupName, newDtUpdated);
		};

		Note tmpNote(*this);
		tmpNote.groupName = newGroupName;
		tmpNote.groupId = newGroupId;
		tmpNote.dtLastUpdated = newDtUpdated;
		netClient->RequestToServerWithWait(NetConstants::request_create_note_on_server(), tmpNote.ToStr_v1(), std::move(answFoo));
	}
	// остальные перемещения, т.е. из сетевой куда угодно
	//		(если в локальную - сервер её удалит)
	else if(!currentGroupIsLocal)
	{
		auto answFoo = [this, newGroupId, newGroupName, newGroupIsLocal, newDtUpdated](QString &&answContent){
			if(answContent == NetConstants::success())
			{
				if(!newGroupIsLocal) // если из сетевой в сетевую, просто меняем группу
				{
					UpdateNote_group(newGroupId, newGroupName, newDtUpdated); // если из сетевой в локальную,
				}
				else // а если из сетевой в локальную, ставим ей новый локальный id
				{
					auto newId = DataBase::GetNewIdForLocalNote();
					UpdateNote_id_group(newId.toLongLong(), newGroupId, newGroupName, newDtUpdated);
				}
			}
			else Error("Can't move note to group, bad server answ");
		};

		auto request = NetConstants::MakeRequest_move_note_to_group(QSn(this->id), newGroupId, newDtUpdated);
		netClient->RequestToServerWithWait(NetConstants::request_move_note_to_group(), std::move(request), std::move(answFoo));
	}
	else Error("Note::MoveToGroup impossible case");
}

void Note::UpdateNote_group(const QString &newGroupId, const QString &newGroupName, QDateTime newDtLastUpdated)
{
	if(!DataBase::MoveNoteToGroup(QSn(id), newGroupId, newDtLastUpdated.toString(Fields::dtFormatLastUpdated())))
	{
		Error("DataBase::MoveNoteToGroup returned false; tryed move to " + newGroupName);
		return;
	}

	groupName = newGroupName;
	groupId = newGroupId;
	dtLastUpdated = newDtLastUpdated;
	EmitCbs(cbsGroupChanged);
}

void Note::UpdateNote_id_group(qint64 newNoteId, QString newGroupId, QString newGroupName, QDateTime newDtLastUpdated)
{
	auto saveRes = DataBase::UpdateNote_IdNote_IdGroup(QSn(id), QSn(newNoteId), newGroupId, dtLastUpdated);
	if(saveRes.isEmpty())
	{
		id = newNoteId;
		groupName = std::move(newGroupName);
		groupId = std::move(newGroupId);
		dtLastUpdated = newDtLastUpdated;
		EmitCbs(cbsGroupChanged);
	}
	else Error("SaveNoteOnClient_IdNote_IdGroup error: " + saveRes);
}

Note Note::Clone() const
{
	Note note;
	note.name = name;
	note.activeNotify = activeNotify;
	note.dtCreated = dtCreated;
	note.dtNotify = dtNotify;
	note.dtPostpone = dtPostpone;
	note.groupName = groupName;
	note.groupId = groupId;
	note.id = id;
	note.content = content;
	note.dtLastUpdated = dtLastUpdated;
	return note;
}

void Note::InitFromTmpNote(Note &note)
{
	name = std::move(note.name);
	activeNotify = note.activeNotify;
	dtCreated = std::move(note.dtCreated);
	dtNotify = std::move(note.dtNotify);
	dtPostpone = std::move(note.dtPostpone);
	groupName = note.groupName;
	groupId = note.groupId;
	id = note.id;
	content = std::move(note.content);
	dtLastUpdated = std::move(note.dtLastUpdated);
}

void Note::InitFromRecord(QStringList &row)
{
	name			= std::move(row[Fields::nameNoteInd]);
	activeNotify	= row[Fields::activeNotifyInd].toInt();
	dtCreated		= QDateTime::fromString(row[Fields::dtCreatedInd], Fields::dtFormat());
	dtNotify		= QDateTime::fromString(row[Fields::dtNotifyInd], Fields::dtFormat());
	dtPostpone		= QDateTime::fromString(row[Fields::dtPostponeInd], Fields::dtFormat());
	groupName		= DataBase::GroupName(row[Fields::idGroupIndInNotes]);
	groupId			= row[Fields::idGroupIndInNotes];
	id				= row[Fields::idNoteInd].toInt();
	content			= std::move(row[Fields::contentInd]);
	dtLastUpdated	= QDateTime::fromString(row[Fields::dtLastUpdatedInd], Fields::dtFormatLastUpdated());
}

QStringList Note::SaveToRecord() const
{
	QStringList rec = MyQString::SizedQStringList(Fields::fieldsInRecCount);
	rec[Fields::nameNoteInd]		 = name			;
	rec[Fields::activeNotifyInd]	 = Fields::LogicValueFromBool(activeNotify)	;
	rec[Fields::dtCreatedInd]		 = DTCreatedStr()		;
	rec[Fields::dtNotifyInd]		 = DTNotifyStr()		;
	rec[Fields::dtPostponeInd]		 = DTPostponeStr()		;
	rec[Fields::idGroupIndInNotes]	 = groupName		;
	rec[Fields::idGroupIndInNotes]	 = groupId		;
	rec[Fields::idNoteInd]			 = QSn(id)				;
	rec[Fields::contentInd]			 = content		;
	rec[Fields::dtLastUpdatedInd]		 = DtLastUpdatedStr()	;
	return rec;
}

Note Note::CreateFromRecord(QStringList & record)
{
	Note note;
	note.InitFromRecord(record);
	return note;
}

std::shared_ptr<Note> Note::CreateFromRecord_shptr(QStringList &record)
{
	std::shared_ptr<Note> note(new Note);
	note->InitFromRecord(record);
	return note;
}

QString Note::InitFromRecordAndSaveToStr(QStringList &record)
{
	Note n;
	n.InitFromRecord(record);
	if(0) CodeMarkers::can_be_optimized("can do this without InitFromRecord and ToStr_v1");
	return n.ToStr_v1();
}

void Note::UpdateThisNoteFromSQL()
{
	if(DataBase::CountNoteId(QSn(id)) != 1) { Error("UpdateThisNoteFromSQL: bad CountNoteId = "+QSn(DataBase::CountNoteId(QSn(id)))); return; }

	auto rec = DataBase::NoteById(QSn(id));
	if(rec.isEmpty()) { Error("UpdateThisNoteFromSQL rec isEmpty"); return; }
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

void Note::SetDTPostpone(QDateTime dtPostpone)
{
	this->dtPostpone = std::move(dtPostpone);
	for(uint i=0; i<cbsDTUpdated.size(); i++)
	{
		auto &cb = cbsDTUpdated[i];
		cb.cb(cb.handler);
	}
}

QString Note::Name_DTNotify_DTPospone() const
{
	QString str;
	static const int addSize = + 2 + strlen(DateTimeFormat_rus) + 3 + strlen(DateTimeFormat_rus) + 1 + 10;
	int size = name.size() + addSize;
	str.reserve(size);
	str.append(name).append(" (").append(dtNotify.toString(DateTimeFormat_rus)).append(" | ")
			.append(dtPostpone.toString(DateTimeFormat_rus)).append(")");
	if(str.size() > size)
	{
		DO_ONCE(QMbError("Name_DTNot_DTPosp: fakt size: "+QSn(str.size())+" istead of "+QSn(size)));
	}
	return str;
}

bool Note::CheckNoteForFilters(const QString &textFilter, const QString &textFilterTranslited)
{
	if(textFilter.isEmpty() && textFilterTranslited.isEmpty()) return true;

	QStringRef contentHead(&content, 0, content.size() > 2000 ? 2000 : content.size());

	if(name.contains(textFilter, Qt::CaseInsensitive)
			|| name.contains(textFilterTranslited, Qt::CaseInsensitive)
			|| contentHead.contains(textFilter, Qt::CaseInsensitive)
			|| contentHead.contains(textFilterTranslited, Qt::CaseInsensitive))
		return true;
	else return false;
}

void Note::SaveNoteOnClient(const QString &reason)
{
	qdbg << "SaveNote for "+reason+" name:" + name;

	dtLastUpdated = QDateTime::currentDateTime();

	auto saveRes = DataBase::UpdateRecordFromNote(this);
	if(!saveRes.isEmpty())
	{
		QMbError("Saving note error: " + saveRes);
		return;
	}

	if(!DataBase::IsGroupLocalById(groupId))
	{
		SendNoteSavedToServer(QSn(id), true);
	}

	if(!timerResaverChecker) QMbError("invalid timerResaver");
}

void Note::SendNoteSavedToServer(QString id, bool showWarningIfServerNotConnected)
{
	if(auto count = DataBase::CountNoteId(id); count == 0)
	{
		// заметка была удалена, отправлять на сервер её больше не нужно
		return;
	}
	else if(count == 1) {}
	else { QMbError("SendNoteSavedToServer bad idOnServer count " + QSn(count)); return; }

	DataBase::SetNoteNotSendedToServer(id, true);

	NetClient::AnswerWorkerFunction answFoo = [id](QString &&answContent){
		if(answContent == NetConstants::success())
		{
			if(DataBase::CountNoteId(id) == 1)
				DataBase::SetNoteNotSendedToServer(id, false);
			//else note now not exists, nothing to do
		}
		else QMbWarning("Server can't save note, it saved local and will try send to server later");
	};

	Note note = DataBase::NoteById_make_note(id);

	if(netClient->sessionId > 0)
		netClient->RequestToServerWithWait(NetConstants::request_note_saved(), note.ToStr_v1(), answFoo);
	else
	{
		if(showWarningIfServerNotConnected)
			QMbWarning("Server not connected, note saved local and will try send to server later");
	}
}

std::unique_ptr<Note> Note::LoadNote(const QString &text)
{
	if(!text.startsWith(SaveKeyWods::version())) { Error("Wrong text to load note"); return {}; }

	int v = GetVersion(text);
	if(loadsFunctionsMap.empty()) InitLoadsFooMap();
	if(auto it = loadsFunctionsMap.find(v); it != loadsFunctionsMap.end())
	{
		auto &loadFunctoin = it->second;
		auto noteUptr = std::make_unique<Note>(loadFunctoin(text));
		if(!noteUptr) Error("loadFunctoin from loadsFunctionsMap returned nullptr");
		return noteUptr;
	}
	else Error("Not found load function for version "+QSn(v));

	return {};
}

Note Note::FromStr_v1(const QString &text)
{
	auto fileParts = text.split(SaveKeyWods::endValue());
	if(fileParts.size() < 12) { Error("Wrong file content (size="+QSn(fileParts.size())+")"); return {}; }

	Note newNote;
	int index = 1;
	newNote.name = std::move(fileParts[index++]);
	newNote.id = fileParts[index++].toLongLong();
	newNote.activeNotify = fileParts[index++].toLongLong();
	newNote.SetDTCreatedFromStr(fileParts[index++]);
	newNote.dtNotify = QDateTime().fromString(fileParts[index++], Fields::dtFormat());
	newNote.dtPostpone = QDateTime().fromString(fileParts[index++], Fields::dtFormat());
	newNote.groupName = std::move(fileParts[index++]);
	newNote.groupId = std::move(fileParts[index++]);
	newNote.content = std::move(fileParts[index++]);
	newNote.dtLastUpdated = QDateTime().fromString(fileParts[index++], Fields::dtFormatLastUpdated());
	return newNote;
}

QString Note::ToStr_v1() const
{
	QString noteText;
	noteText.append(SaveKeyWods::version()).append("1").append(SaveKeyWods::endValue());
	noteText.append(name).append(SaveKeyWods::endValue());
	noteText.append(QSn(id)).append(SaveKeyWods::endValue());
	noteText.append(QSn(activeNotify)).append(SaveKeyWods::endValue());
	noteText.append(DTCreatedStr()).append(SaveKeyWods::endValue());
	noteText.append(dtNotify.toString(Fields::dtFormat())).append(SaveKeyWods::endValue());
	noteText.append(dtPostpone.toString(Fields::dtFormat())).append(SaveKeyWods::endValue());
	noteText.append(groupName).append(SaveKeyWods::endValue());
	noteText.append(groupId).append(SaveKeyWods::endValue());
	noteText.append(content).append(SaveKeyWods::endValue());
	noteText.append(dtLastUpdated.toString(Fields::dtFormatLastUpdated())).append(SaveKeyWods::endValue());
	return noteText;
}

std::vector<Note> Note::LoadNotes()
{
	auto notes = DataBase::NotesFromBD(true);
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

void Note::ExecRemoveNoteWorker(bool execSqlRemove)
{
	for(uint i=0; i<cbsBeforeRemoved.size(); i++)
	{
		auto &cb = cbsBeforeRemoved[i];
		cb.cb(cb.handler);
	}

	if(removeNoteWorker) removeNoteWorker(execSqlRemove);
	else qdbg << "Note::Remove() execed, but removeWorker not valid";
}

qint64 Note::SecsToAlarm(const QDateTime & dateToCompare)
{
	return dateToCompare.secsTo(dtPostpone);
}

void Note::ShowMenuFastActions(QWidget *widgetToShowUnder)
{
	if(0) CodeMarkers::to_do("сделать нормально извлечение текста, а не через костыль QTextEdit");
	QTextEdit te;
	te.setHtml(this->Content());

	auto actions = FastActions::Scan(te.toPlainText());

	if(!actions.actionsVals.isEmpty())
		MyQDialogs::MenuUnderWidget(widgetToShowUnder, actions.actionsVals, actions.GetVectFunctions());
	else MyQDialogs::MenuUnderWidget(widgetToShowUnder, { MyQDialogs::DisabledItem("Actions not found") });
}

void Note::InitTimerResaverNotSavedNotes(QWidget *parent)
{
	if(!timerResaverChecker)
	{
		timerResaverChecker = new QTimer(parent);

		static QStringList toSendIds;
		QObject::connect(timerResaverChecker, &QTimer::timeout, parent, [](){
			if(!netClient or netClient->sessionId == NetClient::undefinedSessionId) return;

			toSendIds = DataBase::NotesNotSendedToServer();
		});
		timerResaverChecker->start(5000);

		static auto timerSender = new QTimer(parent);
		QObject::connect(timerSender, &QTimer::timeout, parent, [](){
			if(!netClient or netClient->sessionId == NetClient::undefinedSessionId) return;
			if(toSendIds.isEmpty()) return;

			SendNoteSavedToServer(std::move(toSendIds.back()), false);
			toSendIds.removeLast();
		});
		timerSender->start(300);
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

void Note::AddCBBeforeNoteRemoved(std::function<void (void *)> aUpdatedCb, void * handler, int & localCbCounter)
{
	if(aUpdatedCb && handler)
	{
		auto &cbRef = cbsBeforeRemoved.emplace_back();
		cbRef.cb = aUpdatedCb;
		cbRef.handler = handler;
		localCbCounter++;
	}
	else qdbg << "AddCBBeforeNoteRemoved invalid cb or handler";
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
	for(uint i=0; i<cbsBeforeRemoved.size(); i++)
		if(cbsBeforeRemoved[i].handler == handler)
		{
			cbsBeforeRemoved.erase(cbsBeforeRemoved.begin()+i);
			countRemoved++;
		}
	if(removedCountShouldBe != countRemoved)
	{
		qdbg << "RemoveCbs: removedCountShouldBe != countRemoved ("+QSn(removedCountShouldBe)
					+" != "+QSn(countRemoved)+") for note " + name;
		Error("RemoveCbs: removedCountShouldBe != countRemoved ("+QSn(removedCountShouldBe)
					+" != "+QSn(countRemoved)+") for note " + name);
	}
}


