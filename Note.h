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
	QDateTime dtLastUpdated;

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

	qint64 id = -1;
	qint64 idOnServer = -1;
	inline static const int idMarkerCreateNewNote = -2;

	QString ToStrForLog();

	inline static const QString& defaultGroupName2() { static QString str = "defaultGroup"; return str; }
	void DialogMoveToGroup();
	void DialogEditCurrentGroup();
	void DialogCreateNewGroup();

	QString group = defaultGroupName2();
	void MoveToGroup(QString newGroupName);
	void MoveToGroupOnClient(const QString &newGroupId, const QString &newGroupName);

	void InitFromTmpNote(Note &note);
	void InitFromRecord(QStringList &record);
	static Note CreateFromRecord(QStringList &record);
	void UpdateThisNoteFromSQL();

	QString Name() { return name; }
	void SetName(QString newName);

	const QString& Content() { return content; }
	void SetContent(QString content);

	QDateTime DTNotify() { return dtNotify; }
	QDateTime DTPostpone() { return dtPostpone; }
	bool CmpDTs(const QDateTime &dtNotify, const QDateTime &dtPostpone);
	void SetDT(QDateTime dtNotify, QDateTime dtPostpone);

	QString DtLastUpdatedStr() { return dtLastUpdated.toString(Fields::dtFormatLastUpated()); }

	static const QString& StartText() { static QString str = "Введите текст"; return str; }

	void SaveNoteOnClient(const QString &reason);
	static std::unique_ptr<Note> LoadNote(const QString &text);
	static Note FromStr_v1(const QString &text);
	QString ToStr_v1() const;
	static std::vector<Note> LoadNotes();
	static void LoadNotesFromFilesAndSaveInBd();

	inline static std::map<int, std::function<Note(const QString &text)>> loadsFunctionsMap;
	static void InitLoadsFooMap()
	{
		loadsFunctionsMap[1] = [](const QString &text){ return FromStr_v1(text); };
	}
	static int GetVersion(const QString &text);

	void ExecRemoveNoteWorker();
	std::function<void()> removeNoteWorker;

	bool CheckAlarm(const QDateTime &dateToCompare);

	void ShowDialogFastActions(QWidget *widgetToShowUnder);

	inline static NetClient *netClient = nullptr;

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
	static void EmitCbs(const std::vector<cbAndHandler> &cbs) { for(auto &cb:cbs) cb.cb(cb.handler); }
// cbs end
};

class INotesOwner
{
public:
	virtual ~INotesOwner() = default;

	virtual void CreateNewNote() = 0;
	virtual void ShowMainWindow() = 0;
};

#endif // NOTE_H
