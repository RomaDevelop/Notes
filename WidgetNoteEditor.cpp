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

#include "PlatformDependent.h"
#include "MyQDifferent.h"
#include "MyQDialogs.h"

WidgetNoteEditor::WidgetNoteEditor(Note &note, QWidget *parent):
	QWidget(parent),
	note {note}
{
	setWindowTitle(note.name + " - NoteEditor");
	setAttribute(Qt::WA_DeleteOnClose);

	QVBoxLayout *vlo_main = new QVBoxLayout(this);
	QHBoxLayout *hloNameAndDates = new QHBoxLayout;
	QHBoxLayout *hloButtons = new QHBoxLayout;
	QHBoxLayout *hloTextEdit = new QHBoxLayout;

	vlo_main->addLayout(hloNameAndDates);

	leName = new QLineEdit(note.name);
	MyQWidget::SetFontPointSize(leName, 12);
	MyQWidget::SetFontBold(leName, true);

	dtEditNotify = new QDateTimeEdit(note.DTNotify());
	MyQWidget::SetFontPointSize(dtEditNotify, 12);
	MyQWidget::SetFontBold(dtEditNotify, true);
	dtEditNotify->setDisplayFormat("dd.MM.yyyy HH:mm:ss");
	dtEditNotify->setCalendarPopup(true);

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
	hloNameAndDates->addWidget(dtEditPostpone);

	vlo_main->addLayout(hloButtons);
	vlo_main->addLayout(hloTextEdit);

	QPushButton *btnBold = new QPushButton("B");
	btnBold->setFixedWidth(25);
	hloButtons->addWidget(btnBold);
	connect(btnBold,&QPushButton::clicked,[this](){
		auto cursor = textEdit->textCursor();
		auto format = cursor.charFormat();
		format.setFontWeight(QFont::Bold);
		cursor.setCharFormat(format);
	});

	QPushButton *btnUnderlined = new QPushButton("U");
	btnUnderlined->setFixedWidth(25);
	hloButtons->addWidget(btnUnderlined);
	connect(btnUnderlined,&QPushButton::clicked,[this](){
		auto cursor = textEdit->textCursor();
		auto format = cursor.charFormat();
		format.setFontUnderline(true);
		cursor.setCharFormat(format);
	});

	QPushButton *btnAplus = new QPushButton("A+");
	btnAplus->setFixedWidth(30);
	hloButtons->addWidget(btnAplus);
	connect(btnAplus,&QPushButton::clicked,[this](){
		auto cursor = textEdit->textCursor();
		auto format = cursor.charFormat();
		format.setFontPointSize(format.fontPointSize()+1);
		cursor.setCharFormat(format);
	});

	QPushButton *btnAminis = new QPushButton("A-");
	btnAminis->setFixedWidth(30);
	hloButtons->addWidget(btnAminis);
	connect(btnAminis,&QPushButton::clicked,[this](){
		auto cursor = textEdit->textCursor();
		auto format = cursor.charFormat();
		format.setFontPointSize(format.fontPointSize()-1);
		cursor.setCharFormat(format);
	});

	hloButtons->addStretch();

	textEdit = new MyQTextEdit;
	textEdit->richTextPaste = false;
	hloTextEdit->addWidget(textEdit);

	if(note.content.code == Note::StartText()) note.content.code = "<span style='font-size: 14pt;'>"+Note::StartText()+"</span>";
	textEdit->setHtml(note.content.code);

	note.ConnectDTUpdated([this](void *){
		static int i =0;
		qdbg << i++ << "ConnectDTUpdated" << this->note.DTNotify().toString() << this->note.DTPostpone().toString();
		dtEditNotify->setDateTime(this->note.DTNotify());
		dtEditPostpone->setDateTime(this->note.DTPostpone());
	},
	this);

	settingsFile = MyQDifferent::PathToExe()+"/files/settings_note_editor.ini";
	QTimer::singleShot(0,this,[this]
	{
		//move(10,10);
		//resize(1870,675);
		LoadSettings();
	});
}

WidgetNoteEditor::~WidgetNoteEditor()
{
	qdbg << "~WidgetNoteEditor " + leName->text();

	note.RemoveCb(this);

	note.content.code = textEdit->toHtml();
	note.name = leName->text();
	note.SetDT(dtEditNotify->dateTime(), dtEditPostpone->dateTime());
	note.EmitUpdatedCommon();
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
		if(answ == "Да, завершить и сохранить") {/*ничего не делаем*/}
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




