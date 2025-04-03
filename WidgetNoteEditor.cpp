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

WidgetNoteEditor::WidgetNoteEditor(Note &note, QWidget *parent):
	QWidget(parent),
	note {note}
{
	setWindowTitle(note.name + " - NoteEditor");

	QVBoxLayout *vlo_main = new QVBoxLayout(this);
	QHBoxLayout *hloNameAndDates = new QHBoxLayout;
	QHBoxLayout *hloButtons = new QHBoxLayout;
	QHBoxLayout *hloTextEdit = new QHBoxLayout;

	vlo_main->addLayout(hloNameAndDates);

	leName = new QLineEdit(note.name);

	dtEditNotify = new QDateTimeEdit(note.dtNotify);
	dtEditNotify->setDisplayFormat("dd.MM.yyyy HH:mm:ss");
	dtEditNotify->setCalendarPopup(true);

	dtEditPostpone = new QDateTimeEdit(note.dtPostpone);
	dtEditPostpone->setDisplayFormat("dd.MM.yyyy HH:mm:ss");

	connect(dtEditNotify, &QDateTimeEdit::dateTimeChanged, [this](const QDateTime &datetime){
		dtEditPostpone->setDateTime(datetime);
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
	note.content.code = textEdit->toHtml();
	note.name = leName->text();
	note.dtNotify = dtEditNotify->dateTime();
	note.dtPostpone = dtEditPostpone->dateTime();
	note.EmitUpdated();
	if(auto thisEditor = existingEditors.find(&note); thisEditor != existingEditors.end())
	{
		existingEditors.erase(thisEditor);
	}
	else QMbc(0,"Error", "destructor called, but this editor not in the existingEditors");
}

void WidgetNoteEditor::MakeNoteEditor(Note & note)
{
	if(auto existingEditor = existingEditors.find(&note); existingEditor == existingEditors.end())
	{
		auto editor = new WidgetNoteEditor(note);
		editor->show();
		existingEditors[&note] = editor;
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
	//	auto answ = QMessageBox::question(this,"Закрытие ...","...");
	//	if(0){}
	//	else if(answ == QMessageBox::Yes) {/*ничего не делаем*/}
	//	else if(answ == QMessageBox::No) { event->ignore(); return; }
	//	else { qCritical() << "not realesed button 0x" + QString::number(answ,16); return; }

	SaveSettings();
	event->accept();
	this->deleteLater();
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




