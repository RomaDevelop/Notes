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
	QDateTime dtCreated;
	QDateTime dtNotify = QDateTime::currentDateTime();
	QDateTime dtPostpone = QDateTime::currentDateTime();
	QDateTime dtLastUpdated;

public:
	Note() = default;
	Note(QString name_, bool activeNotify_, QDateTime dtNotify_, QDateTime dtPostpone_, QString content_);
	~Note() {}

	qint64 id = std::numeric_limits<qint64>::min();

	QString ToStrForLog();

	inline static void Log(const QString &str) { if(logWorker) logWorker(str); else qdbg << str; }
	inline static void Error(const QString &str) { if(errorWorker) errorWorker(str); else qdbg << str; }

	using logWorkerFunction = std::function<void(const QString &str)>;
	inline static logWorkerFunction logWorker;
	inline static logWorkerFunction errorWorker;

	inline static const QString& defaultGroupName() { static QString str = "defaultGroup"; return str; }
	inline static const QString& defaultGroupId() { static QString str = "0"; return str; }
	void DialogRenameNote();
	void DialogMoveToGroup();
	void DialogEditCurrentGroup();
	void DialogCreateNewGroup();

	QString groupName = defaultGroupName();
	QString groupId = defaultGroupId();
	void MoveToGroup(QString newGroupName);
	void UpdateNote_group(const QString &newGroupId, const QString &newGroupName, QDateTime newDtLastUpdated);
	void UpdateNote_id_group(qint64 newNoteId, QString newGroupId, QString newGroupName, QDateTime newDtLastUpdated);

	Note Clone() const;
	void InitFromTmpNote(Note &note);
	void InitFromRecord(QStringList &record);
	QStringList SaveToRecord() const;
	static				   Note  CreateFromRecord(QStringList &record);
	static std::shared_ptr<Note> CreateFromRecord_shptr(QStringList &record);
	static QString InitFromRecordAndSaveToStr(QStringList &record);
	void UpdateThisNoteFromSQL();

	QString Name() { return name; }
	void SetName(QString newName);

	const QString& Content() { return content; }
	void SetContent(QString content);

	QDateTime DTCreated() const { return dtCreated; }
	QString DTCreatedStr() const { return dtCreated.toString(Fields::dtFormat()); }
	void SetDTCreated(QDateTime dtCreated) { this->dtCreated = std::move(dtCreated); }
	void SetDTCreatedFromStr(QString dtCreated) { this->dtCreated = QDateTime::fromString(dtCreated, Fields::dtFormat()); }

	QDateTime DTNotify() { return dtNotify; }
	QString DTNotifyStr() const { return dtNotify.toString(Fields::dtFormat()); }
	QDateTime DTPostpone() { return dtPostpone; }
	QString DTPostponeStr() const { return dtPostpone.toString(Fields::dtFormat()); }
	bool CmpDTs(const QDateTime &dtNotify, const QDateTime &dtPostpone);
	void SetDT(QDateTime dtNotify, QDateTime dtPostpone);

	QDateTime DtLastUpdated() { return dtLastUpdated; }
	QString DtLastUpdatedStr() const { return dtLastUpdated.toString(Fields::dtFormatLastUpdated()); }
	void SetDtLastUpdated(QDateTime dt) { dtLastUpdated = std::move(dt); }

	static const QString& StartText() { static QString str = "Введите текст"; return str; }

	void SaveNoteOnClient(const QString &reason);
	static void SendNoteSavedToServer(QString id, bool showWarningIfServerNotConnected);
	static std::unique_ptr<Note> LoadNote(const QString &text);
	static Note FromStr_v1(const QString &text);
	QString ToStr_v1() const;
	static std::vector<Note> LoadNotes();

	inline static std::map<int, std::function<Note(const QString &text)>> loadsFunctionsMap;
	static void InitLoadsFooMap()
	{
		loadsFunctionsMap[1] = [](const QString &text){ return FromStr_v1(text); };
	}
	static int GetVersion(const QString &text);

	void ExecRemoveNoteWorker();
	std::function<void()> removeNoteWorker;

	bool CheckAlarm(const QDateTime &dateToCompare);

	void ShowMenuFastActions(QWidget *widgetToShowUnder);

	inline static NetClient *netClient = nullptr;

	inline static QTimer *timerResaverChecker = nullptr;
	static void InitTimerResaverNotSavedNotes(QWidget *parent);

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
	virtual void MostOpenedNotes() = 0;
};

#endif // NOTE_H
