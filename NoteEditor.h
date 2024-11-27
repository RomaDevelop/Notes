#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTextEdit>

#include "Note.h"

class NoteEditor : public QWidget
{
	Q_OBJECT
public:

	Note &note;
	QTextEdit *textEdit;

	explicit NoteEditor(Note &note, QWidget *parent = nullptr);
	~NoteEditor();

private:
	void closeEvent (QCloseEvent *event) override;
	//void resizeEvent(QResizeEvent * event) override { }
	//void moveEvent(QMoveEvent * event) override { }
	QString settingsFile;
	void SaveSettings();
	void LoadSettings();

};

#endif // WIDGET_H
