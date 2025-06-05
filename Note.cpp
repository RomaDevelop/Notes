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
	str.append(dtNotify.toString(DateTimeFormat)).append(" ").append(dtPostpone.toString(DateTimeFormat));
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

	ChangeGroup(res.choosedText);
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

	auto blocker = new InputBlocker(qApp);
	qApp->installEventFilter(blocker);

	QProgressDialog *progress = new QProgressDialog("Получение данных", "", 0, 0);
	QPointer<QProgressDialog> progressQPtr(progress);
	progress->setWindowModality(Qt::ApplicationModal);
	progress->setCancelButton(nullptr);

	if(0) CodeMarkers::to_do("make timer singleShot pool");
	QTimer::singleShot(500, [progress]() { if(!progress->wasCanceled()) progress->show(); });
	QTimer::singleShot(3000, [progress, blocker]()
	{
		if(!progress->wasCanceled()) qApp->removeEventFilter(blocker);
		progress->hide();
		progress->deleteLater();
	});

	NetClient::AnswerWorkerFunction answFoo = [progressQPtr, blocker, newGroupName](NetClient::RequestData &&answData){
		if(progressQPtr)
		{
			qApp->removeEventFilter(blocker);
			progressQPtr->cancel();
			progressQPtr->close();
		}
		else
		{
			netClient->Error("answer get, but to late");
			return;
		}

		netClient->Log("answ get: " + answData.content);
		auto id = NetConstants::GetIdGroupFromAnsw_try_create_group(answData.content);
		if(id < 0) QMbError("error decoding in GetIdGroupFromAnsw_try_create_group, answ is " + QSn(id));
		else
		{
			auto resId = DataBase::TryCreateNewGroup(newGroupName, QSn(id));
			if(resId != id) QMbError("TryCreateNewGroup in local base error, resId differs: " + QSn(resId));
		}
	};

	netClient->RequestToServer(NetConstants::request_try_create_group(), newGroupName, std::move(answFoo));
}

void Note::ChangeGroup(QString groupName)
{
	auto grId = DataBase::GroupId(groupName);
	if(grId.isEmpty()) { QMbError("ChangeGroup:: grId.isEmpty() for group " + groupName); return; }

	if(!DataBase::MoveNoteToGroup(QSn(id), grId)) { QMbError("DataBase::MoveNoteToGroup returned false to " + groupName); return; }

#error
	qdbg << ""
			"при запуске нужно проверять, есть ли на сервере такие заметки каких нет локально"
			""
			"обрабатывать ответы сервера об отказе/успехе создания группы"
			""
			"синхронизация на сервере должна быть отложенная"
			""
			"при измении группы заметки она должна синхронизироваться на сервере"
			"	если группа ранее была дефолт - то на сервере будет для неё новая запись"
			"		сервер передаст id заметки у себя и он будет сохранен локально"
			"		нужно хранить в структуре заметки айди на сервере"
			"	если группа не была дефолт, то запись на сервере должна быть обновлена"
			"		отправить всем клиентам информацию, что обновлена"
			""
			"сохранение заметок"
			"	заметка сохраняется в локальной БД, отправляется асинхронный запрос на сервер о сохранении"
			"	сервер отправляет всем клиентам информацию, что заметка была изменена"
			""
			"удаление заметок"
			"	заметка удаляется из локальной БД, отправляется асинхронный запрос на сервер на удаление"
			"		клиент и сервер сохраняют удалённые заметки каждый в своей корзине"
			"	сервер отправляет всем клиентам информацию, что заметка была удалена"
			""
			"при старте работы"
			"	считываются заметки из локальной бд"
			"	все, что не в дефолтной группе сравниваются с сервером"
			"	нужно хранить дату изменения"
			"	ежели заметка на сервере отсутсвует скорее всего она была удалена"
			"		выводить сообщение клиенту, какие заметки были удалены";

	group = groupName;
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
	group = DataBase::GroupName(row[Fields::idGroupInd]);
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
	noteText.append(SaveKeyWods::group()).append(group).append(SaveKeyWods::endValue());
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
	newNote.ChangeGroup(std::move(fileParts[5]));

	newNote.content = std::move(fileParts[6]);
	return newNote;
}

std::vector<Note> Note::LoadNotes()
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
	notes = DataBase::NotesFromBD();

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
	{
		qdbg << "RemoveCbs: removedCountShouldBe != countRemoved ("+QSn(removedCountShouldBe)
					+" != "+QSn(countRemoved)+") for note " + name;
		QMbError("RemoveCbs: removedCountShouldBe != countRemoved ("+QSn(removedCountShouldBe)
					+" != "+QSn(countRemoved)+") for note " + name);
	}
}


