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
	QLabel* labelCaption1;
	QLabel* labelCaption2;
	void* dymmyHandlerToRemoveCb;
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

	QByteArray geometry;
	QString tableColWidths;

private:
	std::vector<NoteInAlarms> notes;
	NoteInAlarms* FindNote(Note *noteToFind);
	void AddNote(Note* note);
	void SetLabelText(NoteInAlarms & note); // maxLen -1 = no max len
	void RemoveNote(int index);
	void RemoveNote(Note* aNote, bool showError);

	enum menuPostponeCase {changeDtNotify, setPostpone};
	static Note* NoteForPostponeAll() { return nullptr; }
	void ShowMenuPostpone(QPoint pos, menuPostponeCase, Note* note);

	QString settingsFile;
	void showEvent(QShowEvent *event) override;
	void closeEvent (QCloseEvent *event) override;
	void SaveSettings();

	QFont fontForLabels;
	QFontMetrics fontMetrixForLabels;

	void FitColWidth();

protected:
	void resizeEvent(QResizeEvent *event) override;
};

#endif // WIDGETNOTIFIE_H
