#ifndef NOTE_H
#define NOTE_H

#include <vector>
#include <memory>

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
	QDateTime notification = QDateTime::currentDateTime();
	QDateTime notifyReschedule = QDateTime::currentDateTime();
	bool alarm = false;

	QString name;
	HTML content;

};

#endif // NOTE_H
