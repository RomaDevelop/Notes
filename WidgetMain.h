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
	std::vector<Note*> AllNotesVect() { std::vector<Note*> v; for(auto &n:notes) v.push_back(n->note.get()); return v; }

	std::unique_ptr<WidgetAlarms> widgetAlarms;

	NetClient *netClient;

	explicit WidgetMain(QWidget *parent = nullptr);
	~WidgetMain();
	void closeEvent (QCloseEvent *event) override;

private:
	void CreateRow1(QHBoxLayout *hlo1);
	void CreateTableContextMenu();
	void CreateTrayIcon();
	void CreateNotesAlarmChecker();
	void CheckNotesForAlarm();

	bool DialogGroupsSubscribes();

	void SlotTest();
	void SlotMenu(QPushButton *btn);

	QString filesPath = MyQDifferent::PathToExe()+"/files";
	QString settingsFile = filesPath + "/settings.ini";
	QString groupsSubscribesFile = filesPath + "/groups_subscribes.ini";
	QStringList groupsSubscribesValue;

	void SaveSettings();
	void LoadSettings();
	void LoadGroupsSubscribes();
	void LoadNotes();
	int RowOfNote(Note* note);
	Note* NoteOfRow(int row);
	Note* NoteOfCurrentRow();
	NoteInMain* NoteById(qint64 id);
	int NoteIndexInWidgetMainNotes(Note* note, bool showError);

	void SlotCreationNewNote();
	enum newNoteReason { loaded, created };
	Note& MakeNewNote(Note noteSrc, newNoteReason reason);
	int MakeWidgetsForMainTable(NoteInMain &newNote); // returns index
	void UpdateWidgetsFromNote(NoteInMain &note);

	void FilterNotes(const QString &textFilter);

	void RemoveNote(Note* note, bool execSqlRemove);
	bool RemoveNoteSQLOnClient(Note* note);
	void RemoveNoteInMainWidget(Note* note);
	void ClearNotesInWidgetMain();

	void DefaultColsWidths();

	void SlotForNetClientNoteRemoved(qint64 id);
	void SlotForNetClientNoteChangedGroupOrUpdated(qint64 id);
	void SlotForNetClientNewNoteAppeared(qint64 id);

	QString GitExtensionsExe;
	QString ReadAndGetGitExtensionsExe(QString dir, bool showInfoMessageBox);
};


#endif // MAINWIDGET_H
