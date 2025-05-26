#include "WidgetNoteEditor.h"

#include <QDebug>
#include <QCloseEvent>
#include <QMessageBox>
#include <QSettings>
#include <QDir>
#include <QTimer>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDateTimeEdit>
#include <QShortcut>

#include "PlatformDependent.h"
#include "MyQDifferent.h"
#include "MyQDialogs.h"

#include "FastActions.h"

WidgetNoteEditor::WidgetNoteEditor(Note &note, QWidget *parent):
	QWidget(parent),
	note {note}
{
	setWindowTitle(note.Name() + " - NoteEditor");
	setAttribute(Qt::WA_DeleteOnClose);

	QVBoxLayout *vlo_main = new QVBoxLayout(this);
	QHBoxLayout *hloNameAndDates = new QHBoxLayout;
	QHBoxLayout *hloButtons = new QHBoxLayout;
	QHBoxLayout *hloTextEdit = new QHBoxLayout;

	vlo_main->addLayout(hloNameAndDates);

	leName = new QLineEdit(note.Name());
	MyQWidget::SetFontPointSize(leName, 12);
	MyQWidget::SetFontBold(leName, true);

	dtEditNotify = new QDateTimeEdit(note.DTNotify());
	MyQWidget::SetFontPointSize(dtEditNotify, 12);
	MyQWidget::SetFontBold(dtEditNotify, true);
	dtEditNotify->setDisplayFormat("dd.MM.yyyy HH:mm:ss");
	dtEditNotify->setCalendarPopup(true);

	auto btnDtEditNotifyAdd = new QPushButton("...");
	btnDtEditNotifyAdd->setFixedWidth(28);
	connect(btnDtEditNotifyAdd,&QPushButton::clicked, [this, btnDtEditNotifyAdd](){
		MyQDialogs::MenuUnderWidget(btnDtEditNotifyAdd, {
			{"09:00:00", [this](){ dtEditNotify->setTime(QTime(9,0,0)); }},
			{"09:30:00", [this](){ dtEditNotify->setTime(QTime(9,30,0)); }},
			{"10:00:00", [this](){ dtEditNotify->setTime(QTime(10,0,0)); }},
			{"10:30:00", [this](){ dtEditNotify->setTime(QTime(10,30,0)); }},
			{"11:00:00", [this](){ dtEditNotify->setTime(QTime(11,0,0)); }},
			{"11:30:00", [this](){ dtEditNotify->setTime(QTime(11,30,0)); }},
			{"12:00:00", [this](){ dtEditNotify->setTime(QTime(12,0,0)); }},
			{"13:00:00", [this](){ dtEditNotify->setTime(QTime(13,0,0)); }},
			{"14:00:00", [this](){ dtEditNotify->setTime(QTime(14,0,0)); }},
			{"15:00:00", [this](){ dtEditNotify->setTime(QTime(15,0,0)); }},
			{"16:00:00", [this](){ dtEditNotify->setTime(QTime(16,0,0)); }},
			{"17:00:00", [this](){ dtEditNotify->setTime(QTime(17,0,0)); }},
			{"18:00:00", [this](){ dtEditNotify->setTime(QTime(18,0,0)); }},
			{"19:00:00", [this](){ dtEditNotify->setTime(QTime(19,0,0)); }},
			{"20:00:00", [this](){ dtEditNotify->setTime(QTime(20,0,0)); }},
			{"21:00:00", [this](){ dtEditNotify->setTime(QTime(21,0,0)); }},
		});
	});

	dtEditPostpone = new QDateTimeEdit(note.DTPostpone());
	MyQWidget::SetFontPointSize(dtEditPostpone, 12);
	MyQWidget::SetFontBold(dtEditPostpone, true);
	dtEditPostpone->setDisplayFormat("dd.MM.yyyy HH:mm:ss");

	connect(dtEditNotify, &QDateTimeEdit::dateTimeChanged, [this](const QDateTime &datetime){
		dtEditPostpone->setDateTime(datetime);
		dtChanged = true;
	});
	connect(dtEditPostpone, &QDateTimeEdit::dateTimeChanged, [this](const QDateTime &){
		dtChanged = true;
	});

	hloNameAndDates->addWidget(leName);
	hloNameAndDates->addWidget(dtEditNotify);
	hloNameAndDates->addWidget(btnDtEditNotifyAdd);
	hloNameAndDates->addWidget(dtEditPostpone);

	vlo_main->addLayout(hloButtons);
	vlo_main->addLayout(hloTextEdit);

	auto SlotSetBold = [this](){
		auto cursor = textEdit->textCursor();
		if(!cursor.hasSelection()) return;

		int start = cursor.selectionStart();
		QTextCharFormat format = MyQTextEdit::LetterFormat(textEdit, start);

		if(format.fontWeight() == QFont::Weight::Normal)
			format.setFontWeight(QFont::Weight::Bold);
		else format.setFontWeight(QFont::Weight::Normal);

		cursor.setCharFormat(format);

		textEdit->setFocus();
	};
	auto SlotSetUndL = [this](){
		auto cursor = textEdit->textCursor();
		if(!cursor.hasSelection()) return;

		int start = cursor.selectionStart();
		QTextCharFormat format = MyQTextEdit::LetterFormat(textEdit, start);

		format.setFontUnderline(!format.fontUnderline());
		cursor.setCharFormat(format);

		textEdit->setFocus();
	};
	auto SlotSetItal = [this](){
		auto cursor = textEdit->textCursor();
		if(!cursor.hasSelection()) return;

		int start = cursor.selectionStart();
		QTextCharFormat format = MyQTextEdit::LetterFormat(textEdit, start);

		format.setFontItalic(!format.fontItalic());
		cursor.setCharFormat(format);

		textEdit->setFocus();
	};

	QPushButton *btnBold = new QPushButton("B");
	btnBold->setStyleSheet("font-weight: bold;");
	btnBold->setFixedWidth(25);
	hloButtons->addWidget(btnBold);
	connect(btnBold,&QPushButton::clicked, SlotSetBold);
	QShortcut *shortcutCtrlB = new QShortcut(QKeySequence("Ctrl+B"), this);
	connect(shortcutCtrlB, &QShortcut::activated, SlotSetBold);

	QPushButton *btnUnderlined = new QPushButton("U");
	btnUnderlined->setStyleSheet("text-decoration: underline;");
	btnUnderlined->setFixedWidth(25);
	hloButtons->addWidget(btnUnderlined);
	connect(btnUnderlined,&QPushButton::clicked,SlotSetUndL);
	QShortcut *shortcutCtrlU = new QShortcut(QKeySequence("Ctrl+U"), this);
	connect(shortcutCtrlU, &QShortcut::activated, SlotSetUndL);

	QPushButton *btnItalic = new QPushButton("I");
	btnItalic->setStyleSheet("font-style: italic;");
	btnItalic->setFixedWidth(25);
	hloButtons->addWidget(btnItalic);
	connect(btnItalic,&QPushButton::clicked,SlotSetItal);
	QShortcut *shortcutCtrlI = new QShortcut(QKeySequence("Ctrl+I"), this);
	connect(shortcutCtrlI, &QShortcut::activated, SlotSetItal);

	QPushButton *btnAPlus = new QPushButton("A+");
	btnAPlus->setFixedWidth(30);
	hloButtons->addWidget(btnAPlus);
	connect(btnAPlus,&QPushButton::clicked,[this](){
		auto cursor = textEdit->textCursor();
		if(!cursor.hasSelection()) return;

		int start = cursor.selectionStart();
		QTextCharFormat format = MyQTextEdit::LetterFormat(textEdit, start);

		format.setFontPointSize(format.fontPointSize()+1);
		cursor.setCharFormat(format);
		textEdit->setFocus();
	});

	QPushButton *btnAMinus = new QPushButton("A-");
	btnAMinus->setFixedWidth(30);
	hloButtons->addWidget(btnAMinus);
	connect(btnAMinus,&QPushButton::clicked,[this](){
		auto cursor = textEdit->textCursor();
		if(!cursor.hasSelection()) return;

		int start = cursor.selectionStart();
		QTextCharFormat format = MyQTextEdit::LetterFormat(textEdit, start);

		format.setFontPointSize(format.fontPointSize()-1);
		cursor.setCharFormat(format);
		textEdit->setFocus();
	});


	QPushButton *btnAddAction = new QPushButton(" Add action ");
	hloButtons->addWidget(btnAddAction);
	connect(btnAddAction,&QPushButton::clicked,[this, btnAddAction](){

		auto executeFoo = [this](){
			this->textEdit->textCursor().insertText(FastActions_ns::execute);
			textEdit->setFocus();
		};

		MyQDialogs::MenuUnderWidget(btnAddAction,
									{
										{ FastActions_ns::execute, executeFoo },
									});
	});

	QPushButton *btnDoActions = new QPushButton(" Do actions ");
	hloButtons->addWidget(btnDoActions);
	connect(btnDoActions,&QPushButton::clicked,[this, btnDoActions](){
		auto actions = FastActions::Scan(textEdit->toPlainText());

		if(!actions.actionsVals.isEmpty())
			MyQDialogs::MenuUnderWidget(btnDoActions, actions.actionsVals, actions.GetVectFunctions());
		else MyQDialogs::MenuUnderWidget(btnDoActions, { MyQDialogs::DisabledItem("Actions not found") });
	});

	hloButtons->addStretch();

	QPushButton *btnTest = new QPushButton(" Test ");
	hloButtons->addWidget(btnTest);
	connect(btnTest,&QPushButton::clicked, [this, btnDtEditNotifyAdd](){
		qdbg << btnDtEditNotifyAdd->height() << dtEditNotify->height();
	});

	QPushButton *btnSave = new QPushButton(" Save ");
	hloButtons->addWidget(btnSave);
	connect(btnSave,&QPushButton::clicked, this, &WidgetNoteEditor::SaveNoteFromWidgets);

	textEdit = new MyQTextEdit;
	textEdit->richTextPaste = false;
	textEdit->setTabStopDistance(40);
	hloTextEdit->addWidget(textEdit);
	connect(textEdit, &QTextEdit::textChanged, [this](){ textEditChanged=true; });

	if(note.Content() == Note::StartText()) note.SetContent("<span style='font-size: 14pt;'>"+Note::StartText()+"</span>");
	textEdit->document()->setDefaultStyleSheet("p { font-size: 14pt; }");
	textEdit->setHtml(note.Content());

	auto dtUpdateFoo = [this](void *){
		static int i =0;
		qdbg << i++ << "ConnectDTUpdated in WidgetNoteEditor" << this->note.DTNotify().toString() << this->note.DTPostpone().toString();
		dtEditNotify->setDateTime(this->note.DTNotify());
		dtEditPostpone->setDateTime(this->note.DTPostpone());
	};

	note.AddCBDTUpdated(dtUpdateFoo, this, cbCounter);

	settingsFile = MyQDifferent::PathToExe()+"/files/settings_note_editor.ini";
	QTimer::singleShot(0,this,[this]
	{
		for(auto &foo:postShowFunctions)
			foo();
		postShowFunctions.clear();
		LoadSettings();
	});
}

WidgetNoteEditor::~WidgetNoteEditor()
{
	qdbg << "~WidgetNoteEditor " + leName->text();

	note.RemoveCbs(this, cbCounter);

	SaveNoteFromWidgets();

	if(auto thisEditor = existingEditors.find(&note); thisEditor != existingEditors.end())
	{
		existingEditors.erase(thisEditor);
	}
	else QMbc(0,"Error", "destructor called, but this editor not in the existingEditors");
}

void WidgetNoteEditor::MakeOrShowNoteEditor(Note & note, bool aNewNote)
{
	if(auto existingEditor = existingEditors.find(&note); existingEditor == existingEditors.end())
	{
		auto editor = new WidgetNoteEditor(note);
		editor->show();
		existingEditors[&note] = editor;
		editor->newNote = aNewNote;
	}
	else
	{
		if(existingEditor->second->isMinimized())
		{
			existingEditor->second->showNormal();
		}
		else
		{
			existingEditor->second->show();
			PlatformDependent::SetTopMost(existingEditor->second,true);
			PlatformDependent::SetTopMost(existingEditor->second,false);
		}
	}
}

void WidgetNoteEditor::closeEvent(QCloseEvent * event)
{
	if(newNote && !dtChanged)
	{
		auto answ = MyQDialogs::CustomDialog("Сохранение","Завершить редактирование и сохранить заметку с автоназначенным уведомлением на "
											 + note.DTNotify().toString(DateTimeFormat) + " ?",
											 {"Да, завершить и сохранить", "Нет, продолжить редактирование"});
		if(answ == "Да, завершить и сохранить")
		{
			dtEditPostpone->setDateTime(note.DTNotify());   // ставим отложить на одну дату с уведомлением
		}
		else if(answ == "Нет, продолжить редактирование") { event->ignore(); return; }
		else { QMbError("not realesed button " + answ); return; }
	}

	SaveSettings();
	event->accept();
}

void WidgetNoteEditor::SaveSettings()
{
	QDir().mkpath(MyQDifferent::PathToExe()+"/files");

	QSettings settings(settingsFile, QSettings::IniFormat);

	settings.setValue("geoNoteEditor", saveGeometry());
}

void WidgetNoteEditor::LoadSettings()
{
	if(!QFile::exists(settingsFile)) return;

	QSettings settings(settingsFile, QSettings::IniFormat);

	this->restoreGeometry(settings.value("geoNoteEditor").toByteArray());
}

void WidgetNoteEditor::SaveNoteFromWidgets()
{
	if(textEditChanged) note.SetContent(textEdit->toHtml());

	QString newName = leName->text();
	if(note.Name() != newName) note.SetName(newName);

	auto dtN = dtEditNotify->dateTime(), dtP = dtEditPostpone->dateTime();
	if(!note.CmpDTs(dtN, dtP))note.SetDT(dtN, dtP);
}




