#include "Note.h"

#include <QDebug>

#include "MyQShortings.h"
#include "MyCppDifferent.h"
#include "MyQFileDir.h"
#include "MyQDialogs.h"
#include "MyQSqlDatabase.h"

const BaseData clientBaseRegular {"Client base",
									"D:\\Documents\\C++ QT\\Notes\\Client base.mdb",
									"D:\\Documents\\C++ QT\\Notes\\storage"};
const BaseData clientBaseDebug {"Client base debug",
									"D:\\Documents\\C++ QT\\Notes\\Client base debug.mdb",
									"D:\\Documents\\C++ QT\\Notes\\storage_debug"};

#ifdef QT_DEBUG
const BaseData clientBase {clientBaseDebug};
#else
const BaseData clientBase {clientBaseRegular};
#endif

namespace SaveKeyWods {

	inline const QString& endValue() { static QString str = "[endValue]\n"; return str; }
	inline const QString& dtFormat() { static QString str = "yyyy.MM.dd hh:mm:ss"; return str; }

	inline const QString& version() { static QString str = "version: "; return str; }
	inline const QString& name() { static QString str = "name: "; return str; }
	inline const QString& activeNotify() { static QString str = "activeNotify: "; return str; }
	inline const QString& dtNotify() { static QString str = "dtNotify: "; return str; }
	inline const QString& dtPostpone() { static QString str = "dtPostpone: "; return str; }
	inline const QString& group() { static QString str = "group: "; return str; }
	inline const QString& content() { static QString str = "content: "; return str; }
}

namespace Fields {
	inline static const QString& Notes() { static QString str = "Notes"; return str; }
	inline static const QString& Groups() { static QString str = "Groups"; return str; }

	inline static const QString& nameGroup() { static QString str = "nameGroup"; return str; }

	inline static const QString& idNote			() { static QString str = "idNote"; return str; }
	//inline static const QString& idNoteOnServer	() { static QString str = "idNoteOnServer"; return str; }
	inline static const QString& idGroup		() { static QString str = "idGroup"; return str; }
	inline static const QString& nameNote		() { static QString str = "nameNote"; return str; }
	inline static const QString& activeNotify	() { static QString str = "activeNotify"; return str; }
	inline static const QString& dtNotify		() { static QString str = "dtNotify"; return str; }
	inline static const QString& dtPostpone		() { static QString str = "dtPostpone"; return str; }
	inline static const QString& content		() { static QString str = "content"; return str; }

	const int idNoteInd			= 0;
	const int idNoteOnServerInd	= idNoteInd+1;
	const int idGroupInd		= idNoteOnServerInd+1;
	const int nameNoteInd		= idGroupInd+1;
	const int activeNotifyInd	= nameNoteInd+1;
	const int dtNotifyInd		= activeNotifyInd+1;
	const int dtPostponeInd		= dtNotifyInd+1;
	const int contentInd		= dtPostponeInd+1;

	inline const QString& dtFormat() { static QString str = SaveKeyWods::dtFormat(); return str; }
}

class DataBase: public MyQSqlDatabase
{
public:
	static void BackupBase()
	{
		QString dtStr = QDateTime::currentDateTime().toString(DateTimeFormatForFileName);
		if(!QFile::copy(baseDataCurrent->baseFilePathName, Note::notesBackupsPath + "/" + dtStr + " " + baseDataCurrent->baseFileNoPath))
			QMbError("creation backup error for base  " + baseDataCurrent->baseFilePathName);
	}

	static QString GroupId(const QString &groupName)
	{
		return DoSqlQueryGetFirstCell("select " + Fields::idGroup() + " from " + Fields::Groups() + "\n"
									  "where " + Fields::nameGroup() + " = :nameGroup",
									  {{":nameGroup",groupName}});
	}
	static const QString& DefaultGroupId()
	{
		static QString str = GroupId(Note::defaultGroupMarker());
		return str;
	}
	static int InsertNoteInDefaultGroup(Note *note)
	{
		auto res = DoSqlQueryExt("insert into " + Fields::Notes() + " ("+Fields::idGroup() + ", " + Fields::nameNote() + ", "
				   + Fields::activeNotify() + ", " + Fields::dtNotify() + ", " + Fields::dtPostpone() + ", " + Fields::content() + ")\n"
				   + "values (:groupId, :name, :actNotif, :dtNotif, :dtPosp, :content)",
				   {{":groupId", DefaultGroupId()}, {":name", note->Name()}, {":actNotif", QSn(note->activeNotify)},
					{":dtNotif", note->DTNotify().toString(Fields::dtFormat())},
					{":dtPosp", note->DTPostpone().toString(Fields::dtFormat())},
					{":content", note->Content()}});

		note->id = -1;

		if(res.errors.isEmpty())
		{
			auto id = DoSqlQueryGetFirstCell("select max("+Fields::idNote()+") from " + Fields::Notes());
			bool ok;
			note->id = id.toUInt(&ok);
			if(!ok)
			{
				note->id = -1;
				QMbError("bad id " + id);
			}

		}
		else QMbError("note was not inserted");

		return note->id;
	}
	static QStringList NoteById(const QString &id)
	{
		return DoSqlQueryGetFirstRec("select * from "+Fields::Notes()+" where "+Fields::idNote()+" = " + id);
	}
	static bool CheckNoteId(const QString &id)
	{
		return DoSqlQueryGetFirstCell("select count("+Fields::idNote()+") from "+Fields::Notes()
									  +" where "+Fields::idNote()+" = " + id).toInt();
	}
	static std::vector<Note> NotesFromDefaultGroup()
	{
		std::vector<Note> notes;
		auto table = DoSqlQueryGetTable("select * from "+Fields::Notes()+" where "+Fields::idGroup()+" = " + DefaultGroupId());
		for(auto &row:table)
		{
			notes.emplace_back();
			notes.back().InitFromRecord(row);
		}
		return notes;
	}
	static void SaveNote(Note *note)
	{
		if(note->group != note->defaultGroup.get()) { QMbError("not default groups unrealesed. Not Saved"); return; }

		// если это новая
		if(note->id == Note::idMarkerCreateNewNote)
		{
			InsertNoteInDefaultGroup(note);
			if(!CheckNoteId(QSn(note->id))) QMbError("SaveNote: note "+note->Name()+" save error");
		}
		else // если не новая
		{
			if(!CheckNoteId(QSn(note->id)))
			{
				QMbError("SaveNote: note with id "+QSn(note->id)+" doesnt exists");
				return;
			}

			auto res = DoSqlQueryExt("update "+Fields::Notes()+" set "+Fields::nameNote() + " = :name, " + Fields::activeNotify()
									 + " = :actNotif, " +Fields::dtNotify()+" = :dtNotif, "+Fields::dtPostpone()
									 + " = :dtPosp, " + Fields::content() + " = :content\n"
																			"where "+Fields::idNote()+" = :idNote",
									 {{":idNote", QSn(note->id)}, {":name", note->Name()}, {":actNotif", QSn(note->activeNotify)},
									  {":dtNotif", note->DTNotify().toString(Fields::dtFormat())},
									  {":dtPosp", note->DTPostpone().toString(Fields::dtFormat())},
									  {":content", note->Content()}});
		}
	}
	static bool RemoveNote(Note *note)
	{
		if(note->group != note->defaultGroup.get()) { QMbError("not default groups unrealesed. Not removed"); return false; }

		if(!CheckNoteId(QSn(note->id)))
		{
			QMbError("RemoveNote: note with id "+QSn(note->id)+" doesnt exists");
			return false;
		}

		auto res = DoSqlQueryExt("delete from "+Fields::Notes()+" where "+Fields::idNote()+" = :idNote",
								 {{":idNote", QSn(note->id)}});

		if(CheckNoteId(QSn(note->id)))
		{
			QMbError("RemoveNote: note with id "+QSn(note->id)+" after delete sql continue exist");
			return false;
		}

		return true;
	}
};

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


