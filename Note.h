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
	QDateTime dtNotify = QDateTime::currentDateTime();
	QDateTime dtPostpone = QDateTime::currentDateTime();

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

	void ConnectUpdated(std::function<void()> aUpdatedCb);
	void EmitUpdated();

private:
	std::vector<std::function<void()>> updatedCbs;
};

#endif // NOTE_H
