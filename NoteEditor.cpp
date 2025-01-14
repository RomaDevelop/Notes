#include "NoteEditor.h"

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

#include "MyQWidgetLib.h"
#include "MyQDifferent.h"

NoteEditor::NoteEditor(Note &note, QWidget *parent):
	QWidget(parent),
	note {note}
{
	QVBoxLayout *vlo_main = new QVBoxLayout(this);
	QHBoxLayout *hlo1 = new QHBoxLayout;
	QHBoxLayout *hlo2 = new QHBoxLayout;
	vlo_main->addLayout(hlo1);
	vlo_main->addLayout(hlo2);

	QPushButton *btn1 = new QPushButton("B");
	btn1->setFixedWidth(25);
	hlo1->addWidget(btn1);
	connect(btn1,&QPushButton::clicked,[this](){
		auto cursor = textEdit->textCursor();
		auto format = cursor.charFormat();
		format.setFontWeight(QFont::Bold);
		cursor.setCharFormat(format);
	});

	hlo1->addStretch();

	textEdit = new QTextEdit;
	hlo2->addWidget(textEdit);

	textEdit->setHtml(note.content.code);

	settingsFile = MyQDifferent::PathToExe()+"/files/settings.ini";
	QTimer::singleShot(0,this,[this]
	{
		//move(10,10);
		//resize(1870,675);
		LoadSettings();
	});
}

NoteEditor::~NoteEditor()
{
	if(auto thisEditor = existingEditors.find(&note); thisEditor != existingEditors.end())
	{
		existingEditors.erase(thisEditor);
	}
	else QMbc(0,"Error", "destructor called, but this editor not in the existingEditors");
}

void NoteEditor::MakeNoteEditor(Note & note)
{
	if(auto existingEditor = existingEditors.find(&note); existingEditor == existingEditors.end())
	{
		auto editor = new NoteEditor(note);
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
			MyQWidgetLib::SetTopMost(existingEditor->second,true);
			MyQWidgetLib::SetTopMost(existingEditor->second,false);
		}
	}
}

void NoteEditor::closeEvent(QCloseEvent * event)
{
	//	auto answ = QMessageBox::question(this,"Закрытие ...","...");
	//	if(0){}
	//	else if(answ == QMessageBox::Yes) {/*ничего не делаем*/}
	//	else if(answ == QMessageBox::No) { event->ignore(); return; }
	//	else { qCritical() << "not realesed button 0x" + QString::number(answ,16); return; }

	SaveSettings();
	note.content.code = textEdit->toHtml();
	event->accept();
	this->deleteLater();
}

void NoteEditor::SaveSettings()
{
	QDir().mkpath(MyQDifferent::PathToExe()+"/files");

	QSettings settings(settingsFile, QSettings::IniFormat);

	settings.setValue("geoNoteEditor", this->saveGeometry());
}

void NoteEditor::LoadSettings()
{
	if(!QFile::exists(settingsFile)) return;

	QSettings settings(settingsFile, QSettings::IniFormat);

	this->restoreGeometry(settings.value("geoNoteEditor").toByteArray());
}




