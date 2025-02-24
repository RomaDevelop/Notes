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
#include "WidgetNotifie.h"

class MainWidget : public QWidget
{
	Q_OBJECT
public:
	QTableWidget *table;
	declare_struct_4_fields_no_move(RowView, QTableWidgetItem*, item, QCheckBox*, chBox, QDateTimeEdit*, dteNotify, QDateTimeEdit*, dtePostpone);
	std::vector<RowView> rowViews;
	std::vector<std::unique_ptr<Note>> notes;
	WidgetAlarms widgetAlarms;

	explicit MainWidget(QWidget *parent = nullptr);
	~MainWidget();

private:
	void CreateTrayIcon();
	void CreateNotesChecker();

	void closeEvent (QCloseEvent *event) override;
	//void resizeEvent(QResizeEvent * event) override { }
	//void moveEvent(QMoveEvent * event) override { }
	QString filesPath = MyQDifferent::PathToExe()+"/files";
	QString settingsFile = filesPath + "/settings.ini";
	QString notesSavesPath = filesPath + "/notes";
	QString MakeNameFileToSaveNote(Note * note, int index);
	void WriteNoteToFile(Note * note, const QString &fileName);

	void SaveSettings();
	void LoadSettings();

	int RowOfNote(Note* note);

	Note& MakeNewNote(QString name, bool activeNotify, QDateTime dtNotify, QDateTime dtPostpone, QString content);
	void MakeNewRowInMainTable(Note* newNote);
	void UpdateRowFromNote(Note* note, int row);
	void NoteUpdated(Note* note);

protected:
	void resizeEvent(QResizeEvent *event) override;
};


#endif // MAINWIDGET_H
