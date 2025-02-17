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
	bool alarm = false;

	QString name;
	HTML content;

	void ConnectUpdated(std::function<void()> aUpdated);
	void EmitUpdated();
private:
	std::function<void()> updated;

};

#endif // NOTE_H
