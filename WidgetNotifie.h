#ifndef WIDGETNOTIFIE_H
#define WIDGETNOTIFIE_H

#include <vector>
#include <set>

#include <QTableWidget>
#include <QWidget>

#include "Note.h"

class WidgetAlarms : public QWidget
{
	Q_OBJECT
public:
	QTableWidget *table;

	explicit WidgetAlarms(QWidget *parent = nullptr);
	~WidgetAlarms() = default;
	void GiveNotes(const std::vector<Note*> &givingNotes);

	QByteArray geometry;
	QString tableColWidths;

private:
	std::vector<Note*> notes;
	void AddNote(Note* note);
	void RemoveNote(int index);

	QString settingsFile;
	void showEvent(QShowEvent *event) override;
	void closeEvent (QCloseEvent *event) override;
};

#endif // WIDGETNOTIFIE_H
