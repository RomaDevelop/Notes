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
};

class WidgetAlarms : public QWidget
{
	Q_OBJECT
public:
	QTableWidget *table;

	explicit WidgetAlarms(INotesOwner *aNotesOwner, QFont fontForLabels, QWidget *parent = nullptr);
	~WidgetAlarms();
	void GiveNotes(const std::vector<Note*> &givingNotes);

	QString tableColWidths;

private:
	void CreateBottomRow(QHBoxLayout *hlo);

	std::vector<std::unique_ptr<NoteInAlarms>> notes;
	NoteInAlarms* FindNote(Note *noteToFind);
	void AddNote(Note* note, bool addInTop, bool disableFeatureMessage = false);
	void MoveNoteUp(Note& note);
	void SetLabelText(NoteInAlarms & note);
	void RemoveNoteFromWidgetAlarms(int index);
	void RemoveNoteFromWidgetAlarms(Note* aNote, bool showError);

	enum menuPostponeCase {changeDtNotify, setPostpone};
	static Note* NoteForPostponeAll() { static Note note; return &note; }
	static Note* NoteForPostponeSelected() { static Note note; return &note; }
	void ShowMenuPostpone(QPoint pos, menuPostponeCase, Note* note);
	void SlotPostpone(std::set<Note*> notesToPostpone, int delaySecs, menuPostponeCase caseCurrent);
	std::set<Note*> GetSelectedNotes();

	QString settingsFile;
	void showEvent(QShowEvent *event) override;
	QByteArray geo;

	QFont fontForLabels;
	QFontMetricsF fontMetrixForLabels;

	void FitColWidth();
	void InitFitColWidthTimer();
	bool fitColWidthRequest = false;

	std::vector<Note*> notesToShowMessageForNotify;
	void InitMessageForNotifyTimer();

	INotesOwner *notesOwner;

protected:
	void resizeEvent(QResizeEvent *event) override;
};

#endif // WIDGETNOTIFIE_H
