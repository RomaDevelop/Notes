#ifndef NOTE_H
#define NOTE_H

#include <vector>
#include <memory>
#include <functional>

#include <QDateTime>
#include <QString>
#include <QTimer>

#include "FastActions.h"

struct HTML
{
	QString code;
};

struct Note
{
	bool activeNotify = false;
private:
	QString name;
	HTML content;
	QDateTime dtNotify = QDateTime::currentDateTime();
	QDateTime dtPostpone = QDateTime::currentDateTime();

public:
	QString Name() { return name; }
	void SetName(QString newName);

	QString Content() { return content.code; }
	void SetContent(QString content);

	QDateTime DTNotify() { return dtNotify; }
	QDateTime DTPostpone() { return dtPostpone; }
	bool CmpDTs(const QDateTime &dtNotify, const QDateTime &dtPostpone);
	void SetDT(QDateTime dtNotify, QDateTime dtPostpone);

	int index = -1;

	QString file;

	static const QString& StartText() { static QString str = "Введите текст"; return str; }

	void SaveNote();
	QString MakeNameFileToSaveNote();
	inline static QString notesSavesPath;
	inline static QString notesBackupsPath;

	void RemoveNoteFromBase();
	std::function<void()> removeNoteFromBaseWorker;

	bool CheckAlarm(const QDateTime &dateToCompare);

	void ShowDialogFastActions(QWidget *widgetToShowUnder);

// cbs
	void SetCBNameUpdated(std::function<void(void *handler)> aUpdatedCb, void *handler, int &localCbCounter);
	void SetCBContentUpdated(std::function<void(void *handler)> aUpdatedCb, void *handler, int &localCbCounter);
	void SetCBDTUpdated(std::function<void(void *handler)> aUpdatedCb, void *handler, int &localCbCounter);
	void RemoveCbs(void* handler, int removedCountShouldBe);
private:
	struct cbAndHandler { std::function<void(void* handler)> cb; void* handler; };
	std::vector<cbAndHandler> cbsNameUpdated;
	std::vector<cbAndHandler> cbsContentUpdated;
	std::vector<cbAndHandler> cbsDTUpdated;
// cbs end
};

#endif // NOTE_H
