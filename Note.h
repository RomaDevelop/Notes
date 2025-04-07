#ifndef NOTE_H
#define NOTE_H

#include <vector>
#include <memory>
#include <functional>

#include <QDateTime>
#include <QString>
#include <QTimer>

struct HTML
{
	QString code;
};

struct Note
{
	bool activeNotify = false;
private:
	QDateTime dtNotify = QDateTime::currentDateTime();
	QDateTime dtPostpone = QDateTime::currentDateTime();
public:
	QDateTime DTNotify() { return dtNotify; }
	QDateTime DTPostpone() { return dtPostpone; }
	void SetDT(QDateTime dtNotify, QDateTime dtPostpone);

	int index = -1;
	QString name;
	QString file;
	HTML content;
	static const QString& StartText() { static QString str = "Введите текст"; return str; }

	void SaveNote();
	QString MakeNameFileToSaveNote();
	inline static QString notesSavesPath;
	inline static QString notesBackupsPath;

	void Remove();
	std::function<void()> removeWorker;

	bool CheckAlarm(const QDateTime &dateToCompare);

	void ConnectCommonUpdated(std::function<void()> aUpdatedCb);
	void ConnectCommonUpdated(std::function<void(void *handler)> aUpdatedCb, void *handler);
	void ConnectDTUpdated(std::function<void(void *handler)> aUpdatedCb, void *handler);
	bool RemoveCb(void* handler);
	void EmitUpdatedCommon();

private:
	std::vector<std::function<void()>> updatedCbs;

	struct cbAndHandler { std::function<void(void* handler)> cb; void* handler; };
	std::vector<cbAndHandler> updatedCbs2;

	std::vector<cbAndHandler> dtUpdatedCbs;
};

#endif // NOTE_H
