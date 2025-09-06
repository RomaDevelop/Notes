#ifndef WIDGETNOTIFIE_H
#define WIDGETNOTIFIE_H

#include <vector>
#include <set>

#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QWidget>

#include "Note.h"

struct NoteInAlarms
{
	Note* note;
	QWidget *widgetAll;
	QWidget *widgetAllExeptLabels;
	QLabel* labelName;
	QLabel* labelDots;
	QLabel* labelDate;
	int cbCounter = 0;

	NoteInAlarms() { validNotesInAlarms.insert(this); }
	~NoteInAlarms() { validNotesInAlarms.erase(this); }

	inline static std::set<NoteInAlarms*> validNotesInAlarms;
};

class WidgetAlarms : public QWidget
{
	Q_OBJECT
public:

	explicit WidgetAlarms(INotesOwner *aNotesOwner, QFont fontForLabels, QWidget *parent = nullptr);
	~WidgetAlarms();
	void AlarmNotes(const std::vector<Note*> &notesToAlarm, Note *nextAlarmNote);

private:
	QTableWidget *table;
	QLabel *labelNextAlarm;

	void CreateTableContextMenu();
	void CreateBottomRow(QBoxLayout *loMain);
	void CreateFindSection(QBoxLayout *loMain);
	std::function<void()> SlotBtnFindClicked = [](){ QMbError("null ShowFindSection"); };

	std::vector<std::unique_ptr<NoteInAlarms>> notes;
	NoteInAlarms* FindNote(Note *noteToFind);
	void AddNote(Note* note, bool addInTop, bool disableFeatureMessage = false);
	void MoveNoteUp(Note& note);
	void SetLabelText(NoteInAlarms & note);
	int NoteIndex(Note* note); // returns -1 if not found
	void RemoveNoteFromWidgetAlarms(int index);
	void RemoveNoteFromWidgetAlarms(Note* aNote, bool showError);

	enum menuPostponeCase {changeDtNotify, setPostpone};
	static Note* NoteForPostponeAll() { static Note note; return &note; }
	static Note* NoteForPostponeSelected() { static Note note; return &note; }
	void ShowMenuPostpone(QPoint pos, menuPostponeCase, Note* note);
	void SlotPostpone(std::set<Note*> notesToPostpone, int delaySecs, menuPostponeCase caseCurrent);

	Note* NoteOfCurrentRow(bool getFirstIfNoSelected);
	std::set<Note*> GetSelectedNotes();

	QString settingsFile;
	void showEvent(QShowEvent *event) override;
	QByteArray geo;

	QFont fontForLabels;
	QFontMetricsF fontMetrixForLabels;

	void FitColWidth();
	void InitFitColWidthTimer();
	bool fitColWidthRequest = false;

	std::vector<NoteInAlarms*> notesToSetLabel;
	QDateTime setLabelRequestDt;
	void InitTimerSetterLabels();

	std::vector<Note*> notesToShowMessageForNotify;
	void InitMessageForNotifyTimer();

	INotesOwner *notesOwner;

protected:
	void resizeEvent(QResizeEvent *event) override;
};

#endif // WIDGETNOTIFIE_H
