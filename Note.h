#ifndef NOTE_H
#define NOTE_H

#include <QDateTime>
#include <QString>

struct HTML
{
	QString code;
};

struct Note
{
	bool activeNotify = false;
	QDateTime notification = QDateTime::currentDateTime();
	QDateTime notifyReschedule = QDateTime::currentDateTime();
	QString name;
	HTML content;
};

#endif // NOTE_H
