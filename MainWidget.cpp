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
#include <QDateTimeEdit>

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
		QString newName = MyQDialogs::InputText("Введите название заметки", "", 400, 150);
		if(newName.isEmpty()) return;

		notes.emplace_back(std::make_unique<Note>());
		auto newNote = notes.back().get();
		newNote->name = std::move(newName);
		table->setRowCount(table->rowCount()+1);
		table->setItem(table->rowCount()-1, 0, new QTableWidgetItem);
		table->item(table->rowCount()-1, 0)->setText(newNote->name);

		CreateNotifyEditor(newNote, table->rowCount()-1);
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
	table->setColumnCount(2);
	hlo2->addWidget(table);
	connect(table, &QTableWidget::cellDoubleClicked, [this](int r, int){
		auto editor = new NoteEditor(*notes[r].get());
		editor->show();
	});

	settingsFile = MyQDifferent::PathToExe()+"/files/settings.ini";
	QTimer::singleShot(0,this,[this]
	{
		LoadSettings();
		for(auto &note:notes)
		{
			table->setRowCount(table->rowCount()+1);
			table->setItem(table->rowCount()-1, 0, new QTableWidgetItem);
			table->item(table->rowCount()-1, 0)->setText(note->name);

			CreateNotifyEditor(note.get(), table->rowCount()-1);
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


	MyQFileDir::WriteFile(settingsFile, "");
	QSettings settings(settingsFile, QSettings::IniFormat);
	settings.setValue("geoMainWidget", this->saveGeometry());
	QString widths;
	for(int i=0; i<table->columnCount(); i++) widths += QSn(table->columnWidth(i)) += ";";
	settings.setValue("columnWidths", widths);

	settings.beginGroup("notes");
	auto keys = settings.childKeys();
	for(auto &note:notes)
	{
		settings.beginGroup(note->name);
		settings.setValue("content", note->content.code);
		settings.setValue("notification", note->notification);
		settings.endGroup();
	}
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
	for(int i=0; i<table->columnCount() && i < widths.size(); i++) table->setColumnWidth(i, widths[i].toUInt());

	settings.beginGroup("notes");
	auto groups = settings.childGroups();
	for(auto &group:groups)
	{
		settings.beginGroup(group);
		auto &newNote = notes.emplace_back(std::make_unique<Note>());
		newNote->name = group;
		newNote->content.code = settings.value("content").toString();
		newNote->notification = settings.value("notification").toDateTime();
		settings.endGroup();
	}
	settings.endGroup();
}

void MainWidget::CreateNotifyEditor(Note * noteToConnect, int rowIndex)
{
	auto dtEdit = new QDateTimeEdit(noteToConnect->notification);
	dtEdit->setDisplayFormat("dd.MM.yyyy HH:mm");
	dtEdit->setCalendarPopup(true);
	table->setCellWidget(rowIndex, 1, dtEdit);
	connect(dtEdit, &QDateTimeEdit::dateTimeChanged, [noteToConnect](const QDateTime &datetime){
		noteToConnect->notification = datetime;
	});
}




