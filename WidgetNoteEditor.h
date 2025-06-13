#ifndef NoteEditor_H
#define NoteEditor_H

#include <set>

#include <QWidget>
#include <QTextEdit>
#include <QLineEdit>

#include "Note.h"
#include "MyQTextEdit.h"

class WidgetNoteEditor : public QWidget
{
	Q_OBJECT

public:
	static void MakeOrShowNoteEditor(Note &note, bool aNewNote = false);

private:
	explicit WidgetNoteEditor(Note &note, bool aNewNote = false, QWidget *parent = nullptr);
public:
	~WidgetNoteEditor();
private:
	void closeEvent (QCloseEvent *event) override;
	//void resizeEvent(QResizeEvent * event) override { }
	//void moveEvent(QMoveEvent * event) override { }
	QString settingsFile;
	void SaveSettings();
	void LoadSettings();

	std::vector<std::function<void()>> postShowFunctions;

	Note &note;
	int cbCounter = 0;
	bool newNote = false;
	bool dtChanged = false;
	QLineEdit *leName;
	QDateTimeEdit *dtEditNotify;
	QDateTimeEdit *dtEditPostpone;
	MyQTextEdit *textEdit;
	bool textEditChanged = false;

	void SaveNoteFromWidgets();

	inline static std::map<Note*,WidgetNoteEditor*> existingEditors;
};

#endif
