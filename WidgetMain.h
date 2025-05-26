#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <memory>

#include <QWidget>
#include <QLayout>
#include <QTextEdit>
#include <QTableWidget>
#include <QDateTime>
#include <QCheckBox>
#include <QDateTimeEdit>
#include <QTcpSocket>

#include "MyQDifferent.h"
#include "declare_struct.h"

#include "NetClient.h"
#include "Note.h"
#include "WidgetAlarms.h"

declare_struct_5_fields_no_move(RowView, QTableWidgetItem*, itemName, QTableWidgetItem*, itemGroup, QCheckBox*, chBox,
											QDateTimeEdit*, dteNotify, QDateTimeEdit*, dtePostpone);
declare_struct_3_fields_move(NoteInMain, RowView, rowView, std::unique_ptr<Note>, note, int, cbCounter);

class WidgetMain : public QWidget
{
	Q_OBJECT
public:
	QTableWidget *table;

	std::vector<std::unique_ptr<NoteInMain>> notes;

	void UpdateNotesIndexes();
	std::unique_ptr<WidgetAlarms> widgetAlarms;

	NetClient *netClient;

	explicit WidgetMain(QWidget *parent = nullptr);
	~WidgetMain();
	void closeEvent (QCloseEvent *event) override;

private:
	void CreateHeaderPanel(QHBoxLayout *hlo1);
	void CreateTableContextMenu();
	void CreateTrayIcon();
	void CreateNotesAlarmChecker();
	void CheckNotesForAlarm();

	void SlotTest();

	QString filesPath = MyQDifferent::PathToExe()+"/files";
	QString settingsFile = filesPath + "/settings.ini";

	void SaveSettings();
	void LoadSettings();
	void LoadNotes();
	int RowOfNote(Note* note);
	Note* NoteOfRow(int row);
	Note* NoteOfCurrentRow();

	void SlotCreationNewNote();
	Note& MakeNewNote(Note noteSrc, bool doSave);
	int MakeWidgetsForMainTable(NoteInMain &newNote); // returns index
	void UpdateWidgetsFromNote(NoteInMain &note);

	void FilterNotes(const QString &nameFilter);

	void RemoveNote(Note* note);

	void DefaultColsWidths();
};


#endif // MAINWIDGET_H
