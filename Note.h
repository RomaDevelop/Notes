#ifndef NOTE_H
#define NOTE_H

#include <vector>
#include <memory>
#include <functional>

#include <QDateTime>
#include <QString>
#include <QTimer>

#include "declare_struct.h"

#include "FastActions.h"
#include "NetClient.h"

using QStringRefWr = std::reference_wrapper<QString>;
using QStringRefWr_const = std::reference_wrapper<const QString>;

struct NotesGroup
{
	QString name;
	QString describtion;
	QDateTime lastUpdate;
};

struct Note
{
	bool activeNotify = false;
private:
	QString name;
	QString content;
	QDateTime dtNotify = QDateTime::currentDateTime();
	QDateTime dtPostpone = QDateTime::currentDateTime();

public:
	Note() = default;
	Note(QString name_, bool activeNotify_, QDateTime dtNotify_, QDateTime dtPostpone_, QString content_):
		activeNotify{activeNotify_},
		name {std::move(name_)},
		dtNotify {std::move(dtNotify_)},
		dtPostpone {std::move(dtPostpone_)}
	{
		content = std::move(content_);
	}

	int index = -1;
	QString file;

	QString ToStrForLog();

	inline static const QString& defaultGroupMarker() { static QString str = "defaultGroup"; return str; }
	inline static std::shared_ptr<NotesGroup> defaultGroup { new NotesGroup {defaultGroupMarker(), "", {}} };
	inline static std::vector<std::shared_ptr<NotesGroup>> groups { defaultGroup };
	inline static std::map<QString, NotesGroup*> groupsMap { std::pair(defaultGroup->name, defaultGroup.get()) };
	static NotesGroup* AddGroup(QString groupName);
	inline static QStringList GroupsNames();
	void DialogMoveToGroup();
	void DialogEditCurrentGroup();
	NotesGroup* DialogCreateNewGroup();

	NotesGroup *group = defaultGroup.get();
	void ChangeGroup(QString groupName, bool createNewIfNeed);
	void ChangeGroup(NotesGroup *newGroup);

	void InitFromTmpNote(Note &note);

	QString Name() { return name; }
	void SetName(QString newName);

	const QString& Content() { return content; }
	void SetContent(QString content);

	QDateTime DTNotify() { return dtNotify; }
	QDateTime DTPostpone() { return dtPostpone; }
	bool CmpDTs(const QDateTime &dtNotify, const QDateTime &dtPostpone);
	void SetDT(QDateTime dtNotify, QDateTime dtPostpone);

	static const QString& StartText() { static QString str = "Введите текст"; return str; }

	void SaveNote(const QString &reason);
	static std::unique_ptr<Note> LoadNote(const QString &text, const QString &fileFrom);
	static Note LoadNote_v1(const QString &text);
	static std::vector<Note> LoadNotes();

	inline static std::map<int, std::function<Note(const QString &text)>> loadsFooMap;
	static void InitLoadsFooMap()
	{
		loadsFooMap[1] = [](const QString &text){ return LoadNote_v1(text); };
	}
	static int GetVersion(const QString &text);
	QString MakeNameFileToSaveNote();
	inline static QString notesSavesPath;
	inline static QString notesBackupsPath;

	void RemoveNoteFromBase();
	std::function<void()> removeNoteFromBaseWorker;

	bool CheckAlarm(const QDateTime &dateToCompare);

	void ShowDialogFastActions(QWidget *widgetToShowUnder);

	inline static NetClient *netClient = nullptr;
	void NetNoteSaved(QString &text);

// cbs
	void AddCBNameUpdated(std::function<void(void *handler)> aUpdatedCb, void *handler, int &localCbCounter);
	void AddCBContentUpdated(std::function<void(void *handler)> aUpdatedCb, void *handler, int &localCbCounter);
	void AddCBDTUpdated(std::function<void(void *handler)> aUpdatedCb, void *handler, int &localCbCounter);
	void AddCBGroupChanged(std::function<void(void *handler)> aUpdatedCb, void *handler, int &localCbCounter);
	void RemoveCbs(void* handler, int removedCountShouldBe);
private:
	struct cbAndHandler { std::function<void(void* handler)> cb; void* handler; };
	std::vector<cbAndHandler> cbsNameUpdated;
	std::vector<cbAndHandler> cbsContentUpdated;
	std::vector<cbAndHandler> cbsDTUpdated;
	std::vector<cbAndHandler> cbsGroupChanged;
// cbs end
};

#endif // NOTE_H
