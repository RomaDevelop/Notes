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
	QLabel* labelCaption1;
	QLabel* labelCaption2;
	int cbCounter = 0;
};

class WidgetAlarms : public QWidget
{
	Q_OBJECT
public:
	QTableWidget *table;

	explicit WidgetAlarms(QFont fontForLabels,
						  std::function<void()> crNewNoteFoo,
						  std::function<void()> showMainWindow,
						  QWidget *parent = nullptr);
	~WidgetAlarms();
	void GiveNotes(const std::vector<Note*> &givingNotes);

	QString tableColWidths;

private:
	std::vector<std::unique_ptr<NoteInAlarms>> notes;
	NoteInAlarms* FindNote(Note *noteToFind);
	void AddNote(Note* note);
	void SetLabelText(NoteInAlarms & note);
	void RemoveNoteFromWidgetAlarms(int index);
	void RemoveNoteFromWidgetAlarms(Note* aNote, bool showError);

	enum menuPostponeCase {changeDtNotify, setPostpone};
	static Note* NoteForPostponeAll() { return nullptr; }
	void ShowMenuPostpone(QPoint pos, menuPostponeCase, Note* note);
	void SlotPostpone(std::vector<Note*> notesToPostpone, int delaySecs, menuPostponeCase caseCurrent);

	QString settingsFile;
	void showEvent(QShowEvent *event) override;
	QByteArray geo;

	QFont fontForLabels;
	QFontMetricsF fontMetrixForLabels;

	void FitColWidth();

protected:
	void resizeEvent(QResizeEvent *event) override;
};

#endif // WIDGETNOTIFIE_H
