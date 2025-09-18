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
	static WidgetNoteEditor* MakeOrShowNoteEditor(Note &note, bool noteIsTmpNote = false);
	static WidgetNoteEditor* MakeOrShowNoteEditorTmpNote(QStringList noteRecord);
	//static WidgetNoteEditor* MakeOrShowNoteEditorTmpNote(Note note);
		// не стоит делать такой способ ибо в Note коллбеки которые могут копироваться

	void StoreTmpNote(std::shared_ptr<Note> tmpNote) { this->tmpNote = tmpNote; }
	void SetReadOnly();

private:
	explicit WidgetNoteEditor(Note &note, QWidget *parent = nullptr);
	void CreateRow2_buttons(QVBoxLayout *lo_main);
	void CreateStatusBar(QLayout *lo_main);
	void InitTimerNoteSaver();
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
	std::shared_ptr<Note> tmpNote;
	int cbCounter = 0;

	bool notChagesNow = false;
	bool haveChanges = false;
	void SetHaveChangesTrue(QString widget);
	QDateTime lastChagesDid;
	QDateTime lastSaveDid;
	int savedCount = 0;

	QLineEdit *leName;
	QDateTimeEdit *dtEditNotify;
	QDateTimeEdit *dtEditPostpone;
	MyQTextEdit *textEdit;

	QLabel *labelStatus;

	void SaveNoteFromEditor(bool forceSave = false);

	inline static std::map<Note*,WidgetNoteEditor*> existingEditors;

};

#endif
