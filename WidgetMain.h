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

class WidgetMain : public QWidget, public INotesOwner
{
	Q_OBJECT
public:
	std::vector<std::unique_ptr<NoteInMain>> notes;
	std::vector<Note*> AllNotesVect() { std::vector<Note*> v; for(auto &n:notes) v.push_back(n->note.get()); return v; }

	std::unique_ptr<WidgetAlarms> widgetAlarms;

	explicit WidgetMain(QWidget *parent = nullptr);
	~WidgetMain();
	void closeEvent (QCloseEvent *event) override;

	virtual void CreateNewNote() override;
	virtual void ShowMainWindow() override;
	virtual void MostOpenedNotes() override;
	virtual std::vector<Note*> Notes(std::function<bool(Note*)> filter = {}) override;
	virtual bool IsNoteValid(Note *note) override;

	Note* FindOriginalNote(qint64 idNote);

private:
	NetClient *netClient;
	QTableWidget *table;

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

	enum newNoteReason { loaded, created };
	Note& MakeNewNote(Note noteSrc);
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
};


#endif // MAINWIDGET_H
