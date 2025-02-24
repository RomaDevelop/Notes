#ifndef WIDGETNOTIFIE_H
#define WIDGETNOTIFIE_H

#include <vector>
#include <set>

#include <QLabel>
#include <QTableWidget>
#include <QWidget>

#include "Note.h"

class WidgetAlarms : public QWidget
{
	Q_OBJECT
public:
	QTableWidget *table;

	explicit WidgetAlarms(QWidget *parent = nullptr);
	~WidgetAlarms();
	void GiveNotes(const std::vector<Note*> &givingNotes);

	QByteArray geometry;
	QString tableColWidths;

private:
	std::vector<Note*> notes;
	void AddNote(Note* note);
	QString GenLabelText(Note* note);
	void RemoveNote(int index);
	void RemoveNote(Note* aNote, bool showError);

	enum menuPostponeCase {changeDtNotify, setPostpone};
	static Note* NoteForPostponeAll() { return nullptr; }
	void ShowMenuPostpone(QPoint pos, menuPostponeCase, Note* note);

	QString settingsFile;
	void showEvent(QShowEvent *event) override;
	void closeEvent (QCloseEvent *event) override;
	void SaveSettings();

protected:
	void resizeEvent(QResizeEvent *event) override;
};

#endif // WIDGETNOTIFIE_H
