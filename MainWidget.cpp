#include "MainWidget.h"

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
#include <QTextEdit>

#include "MyQDifferent.h"
#include "MyQDialogs.h"
#include "MyQFileDir.h"

#include "NoteEditor.h"

MainWidget::MainWidget(QWidget *parent) : QWidget(parent)
{
	QVBoxLayout *vlo_main = new QVBoxLayout(this);
	QHBoxLayout *hlo1 = new QHBoxLayout;
	QHBoxLayout *hlo2 = new QHBoxLayout;
	vlo_main->addLayout(hlo1);
	vlo_main->addLayout(hlo2);

	QPushButton *btn1 = new QPushButton("+");
	btn1->setFixedWidth(25);
	hlo1->addWidget(btn1);
	connect(btn1,&QPushButton::clicked,[this](){
		notes.push_back(std::make_unique<Note>());
		notes.back()->name = MyQDialogs::InputText("Введите название заметки");
		table->setRowCount(table->rowCount()+1);
		table->setItem(table->rowCount()-1, 0, new QTableWidgetItem);
		table->item(table->rowCount()-1, 0)->setText(notes.back()->name);
	});

	QPushButton *btnRemove = new QPushButton("-");
	btnRemove->setFixedWidth(25);
	hlo1->addWidget(btnRemove);
	connect(btnRemove,&QPushButton::clicked,[this](){
		int index = table->currentRow();
		if(table->rowCount() == 0 ||  index < 0 || index >= table->rowCount()) return;
		notes.erase(notes.begin() + index);
		table->removeRow(index);
	});

	QPushButton *btnRename = new QPushButton("Rename");
	btnRename->setFixedWidth(QFontMetrics(btnRename->font()).horizontalAdvance(btnRename->text()) + 20);
	hlo1->addWidget(btnRename);
	connect(btnRename,&QPushButton::clicked,[this](){
		int index = table->currentRow();
		if(table->rowCount() == 0 ||  index < 0 || index >= table->rowCount()) return;
		notes[index]->name = MyQDialogs::InputText("Измените название заметки", notes[index]->name);
		table->item(index, 0)->setText(notes[index]->name);
	});

	hlo1->addStretch();

	table = new QTableWidget;
	table->setColumnCount(1);
	hlo2->addWidget(table);
	connect(table, &QTableWidget::cellDoubleClicked, [this](int r, int){
		auto editor = new NoteEditor(*notes[r].get());
		editor->show();
	});

	settingsFile = MyQDifferent::PathToExe()+"/files/settings.ini";
	QTimer::singleShot(0,this,[this]
	{
		//move(10,10);
		//resize(1870,675);
		LoadSettings();
		for(auto &note:notes)
		{
			table->setRowCount(table->rowCount()+1);
			table->setItem(table->rowCount()-1, 0, new QTableWidgetItem);
			table->item(table->rowCount()-1, 0)->setText(note->name);
		}
	});
}

MainWidget::~MainWidget()
{

}

void MainWidget::closeEvent(QCloseEvent * event)
{
//	auto answ = QMessageBox::question(this,"Закрытие ...","...");
//	if(0){}
//	else if(answ == QMessageBox::Yes) {/*ничего не делаем*/}
//	else if(answ == QMessageBox::No) { event->ignore(); return; }
//	else { qCritical() << "not realesed button 0x" + QString::number(answ,16); return; }

	SaveSettings();
	event->accept();
}

void MainWidget::SaveSettings()
{
	QDir().mkpath(MyQDifferent::PathToExe()+"/files");

	QSettings settings(settingsFile, QSettings::IniFormat);
	settings.setValue("geoMainWidget", this->saveGeometry());
	QString widths;
	for(int i=0; i<table->columnCount(); i++) widths += QSn(table->columnWidth(i)) += ";";
	settings.setValue("columnWidths", widths);

	settings.beginGroup("notes");
	auto keys = settings.childKeys();
	for(auto &key:keys) settings.remove(key);
	for(auto &note:notes) settings.setValue(note->name, note->content.code);
	settings.endGroup();
}

void MainWidget::LoadSettings()
{
	if(!QFile::exists(settingsFile)) return;

	QFileInfo settingsFileInfo(settingsFile);
	QString backubPath = settingsFileInfo.path() + "/settings backups";
	QString backubFile = backubPath + "/" + QDateTime::currentDateTime().toString("yyyy.MM.dd hh_mm_ss") + " " + settingsFileInfo.fileName();
	QDir().mkdir(backubPath);
	if(!QFile::copy(settingsFile, backubFile)) QMbc(nullptr,"error", "can't copy file");
	MyQFileDir::RemoveOldFiles(backubPath, 30);

	QSettings settings(settingsFile, QSettings::IniFormat);

	this->restoreGeometry(settings.value("geoMainWidget").toByteArray());

	QStringList widths = settings.value("columnWidths").toString().split(";", QString::SkipEmptyParts);
	if(table->columnCount() != widths.size()) qdbg << "LoadSettings error table->columnCount() != widths.size()";
	for(int i=0; i<table->columnCount() & i < widths.size(); i++) table->setColumnWidth(i, widths[i].toUInt());

	settings.beginGroup("notes");
	auto keys = settings.childKeys();
	for(auto &key:keys)
	{
		auto &newNote = notes.emplace_back(std::make_unique<Note>());
		newNote->name = key;
		newNote->content.code = settings.value(key).toString();
	}
	settings.endGroup();
}




