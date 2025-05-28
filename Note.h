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
	int id = -1;
	inline static const int idMarkerCreateNewNote = -2;

	QString ToStrForLog();

	inline static const QString& defaultGroupName() { static QString str = "defaultGroup"; return str; }
	void DialogMoveToGroup();
	void DialogEditCurrentGroup();
	void DialogCreateNewGroup();

	QString group = defaultGroupName();
	void ChangeGroup(QString groupName);

	void InitFromTmpNote(Note &note);
	void InitFromRecord(QStringList &record);

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

	inline static std::map<int, std::function<Note(const QString &text)>> loadsFunctionsMap;
	static void InitLoadsFooMap()
	{
		loadsFunctionsMap[1] = [](const QString &text){ return LoadNote_v1(text); };
	}
	static int GetVersion(const QString &text);
	QString MakeNameFileToSaveNote();
	inline static QString notesSavesPath;
	inline static QString notesBackupsPath;

	void ExecRemoveNoteWorker();
	std::function<void()> removeNoteWorker;
	bool RemoveNoteSQL();

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
