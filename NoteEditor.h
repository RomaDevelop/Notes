#ifndef NoteEditor_H
#define NoteEditor_H

#include <set>

#include <QWidget>
#include <QTextEdit>
#include <QLineEdit>

#include "Note.h"
#include "MyQTextEdit.h"

class NoteEditor : public QWidget
{
	Q_OBJECT

public:
	static void MakeNoteEditor(Note &note);
	~NoteEditor();

private:
	explicit NoteEditor(Note &note, QWidget *parent = nullptr);
	void closeEvent (QCloseEvent *event) override;
	//void resizeEvent(QResizeEvent * event) override { }
	//void moveEvent(QMoveEvent * event) override { }
	QString settingsFile;
	void SaveSettings();
	void LoadSettings();

	Note &note;
	QLineEdit *leName;
	MyQTextEdit *textEdit;

	inline static std::map<Note*,NoteEditor*> existingEditors;
};

#endif
