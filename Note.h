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
	QDateTime notification;
	QString name;
	HTML content;
};

#endif // NOTE_H