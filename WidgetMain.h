#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <memory>

#include <QWidget>
#include <QTableWidget>
#include <QDateTime>
#include <QCheckBox>
#include <QDateTimeEdit>

#include "MyQDifferent.h"
#include "declare_struct.h"

#include "Note.h"
#include "WidgetAlarms.h"

declare_struct_4_fields_no_move(RowView, QTableWidgetItem*, item, QCheckBox*, chBox, QDateTimeEdit*, dteNotify, QDateTimeEdit*, dtePostpone);
declare_struct_2_fields_move(NoteInMain, RowView, rowView, std::unique_ptr<Note>, note);

class WidgetMain : public QWidget
{
	Q_OBJECT
public:
	QTableWidget *table;

	std::vector<std::unique_ptr<NoteInMain>> notes;

	void UpdateNotesIndexes();
	std::unique_ptr<WidgetAlarms> widgetAlarms;

	explicit WidgetMain(QWidget *parent = nullptr);
	~WidgetMain();

private:
	void CreateTrayIcon();
	void CreateNotesAlarmChecker();
	void CheckNotesForAlarm();

	void closeEvent (QCloseEvent *event) override;
	//void resizeEvent(QResizeEvent * event) override { }
	//void moveEvent(QMoveEvent * event) override { }
	QString filesPath = MyQDifferent::PathToExe()+"/files";
	QString settingsFile = filesPath + "/settings.ini";

	void SaveSettings();
	void LoadSettings();
	void LoadNotes();

	int RowOfNote(Note* note);
	Note* NoteOfRow(int row);

	Note& MakeNewNote(QString name, bool activeNotify, QDateTime dtNotify, QDateTime dtPostpone, QString content);
	int MakeNewRowInMainTable(NoteInMain &newNote); // returns index
	void UpdateRowFromNote(NoteInMain &note);

	void RemoveNote(Note* note);

	void FitColWidth();

protected:
	void resizeEvent(QResizeEvent *event) override;
};


#endif // MAINWIDGET_H
