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

	QString name;
	QString file;
	HTML content;
	static const QString& StartText() { static QString str = "Введите текст"; return str; }

	bool CheckAlarm(const QDateTime &dateToCompare);

	void ConnectUpdated(std::function<void()> aUpdatedCb);
	void EmitUpdated();


	void ConnectContentUpdated(std::function<void()> cb);
	void EmitContentUpdated();
private:
	std::function<void()> updatedCb;
	std::function<void()> contentUpdatedCb;

};

#endif // NOTE_H
