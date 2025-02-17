#include "MainWidgetNotes.h"

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
#include <QSystemTrayIcon>
#include <QApplication>

#include "MyQDifferent.h"
#include "MyQDialogs.h"
#include "MyQFileDir.h"
#include "MyQExecute.h"
#include "MyQWidgetLib.h"
#include "MyQTableWidget.h"
#include "CodeMarkers.h"

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
		Note* newNote = notes.back().get();
		newNote->name = std::move(newName);

		MakeNewRowInMainTable(newNote);
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

	QPushButton *btnSettings = new QPushButton("Settings.ini");
	btnSettings->setFixedWidth(QFontMetrics(btnSettings->font()).horizontalAdvance(btnSettings->text()) + 20);
	hlo1->addWidget(btnSettings);
	connect(btnSettings,&QPushButton::clicked,[this](){
		MyQExecute::Execute(settingsFile);
	});

	QPushButton *btnToTray = new QPushButton("To tray");
	btnToTray->setFixedWidth(QFontMetrics(btnToTray->font()).horizontalAdvance(btnToTray->text()) + 20);
	hlo1->addWidget(btnToTray);
	connect(btnToTray,&QPushButton::clicked,[this](){
		hide();
	});

	table = new QTableWidget;
	table->setColumnCount(4);
	hlo2->addWidget(table);
	connect(table, &QTableWidget::cellDoubleClicked, [this](int r, int){
		NoteEditor::MakeNoteEditor(*notes[r].get());
	});

	settingsFile = MyQDifferent::PathToExe()+"/files/settings.ini";
	QTimer::singleShot(0,this,[this]
	{
		LoadSettings();
		for(auto &note:notes)
		{
			MakeNewRowInMainTable(note.get());
		}
	});

	CreateTrayIcon();
	CreateNotesChecker();
}

void MainWidget::CreateTrayIcon()
{
	auto icon = new QSystemTrayIcon(this);
	icon->setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowForward));
	icon->setToolTip("Notes");
	icon->show();
	connect(icon, &QSystemTrayIcon::activated, [this](){
		showNormal();
		MyQWidgetLib::SetTopMost(this,true);
		MyQWidgetLib::SetTopMost(this,false);
	});
}

void MainWidget::CreateNotesChecker()
{
	QTimer *tChecher = new QTimer(this);
	connect(tChecher, &QTimer::timeout, [this](){
		if(0) CodeMarkers::to_do("Нужно адекватный алгоритм проверки чтобы не ломалось если будет много задач");
		QDateTime currentDateTime = QDateTime::currentDateTime();
		std::vector<Note*> alarmedNotes;
		for(auto &note:notes)
		{
			if(currentDateTime >= note->dtPostpone)
			{
				note->alarm = true;
				alarmedNotes.emplace_back(note.get());
			}
			else note->alarm = false;
		}

		widgetAlarms.GiveNotes(alarmedNotes);
		if(!alarmedNotes.empty())
			widgetAlarms.show();
		else widgetAlarms.hide();
	});
	tChecher->start(1000);
}

void MainWidget::closeEvent(QCloseEvent * event)
{
	auto answ = MyQDialogs::CustomDialog("Завершение работы приложения","Вы уверены, что хотите завершить работу приложения?"
																		"\n\n(уведомления на задачи не будут поступать)"
																		"\n(можно свернуть в трей, приложение продолжит работать)",
										 {"Завершить", "Свернуть в трей", "Ничего не делать"});
	if(0){}
	else if(answ == "Завершить") {/*ничего не делаем*/}
	else if(answ == "Свернуть в трей") { hide(); event->ignore(); return; }
	else if(answ == "Ничего не делать") { event->ignore(); return; }
	else { QMbc(0,"error", "not realesed button " + answ); event->ignore(); return; }

	SaveSettings();
	event->accept();
	QApplication::exit();
}

void MainWidget::SaveSettings()
{
	QDir().mkpath(QFileInfo(settingsFile).path());
	MyQFileDir::WriteFile(settingsFile, "");
	QSettings settings(settingsFile, QSettings::IniFormat);

	settings.setValue("geoMainWidget", this->saveGeometry());
	QString widths;
	for(int i=0; i<table->columnCount(); i++) widths += QSn(table->columnWidth(i)) += ";";
	settings.setValue("columnWidths", widths);

	settings.beginGroup("notes");
	auto keys = settings.childKeys();
	for(uint i=0; i<notes.size(); i++)
	{
		auto& note = notes[i];
		QString groupName = QSn(i);
		groupName = groupName.rightJustified(5,'0');
		groupName.prepend("note");
		settings.beginGroup(groupName);
		settings.setValue("name", note->name);
		settings.setValue("content", note->content.code);
		settings.setValue("activeNotify", note->activeNotify);
		settings.setValue("notification", note->dtNotify);
		settings.setValue("notifyPostpone", note->dtPostpone);
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

	restoreGeometry(settings.value("geoMainWidget").toByteArray());

	QStringList widths = settings.value("columnWidths").toString().split(";", QString::SkipEmptyParts);
	if(table->columnCount() != widths.size()) qdbg << "LoadSettings error table->columnCount() != widths.size()";
	for(int i=0; i<table->columnCount() && i < widths.size(); i++) table->setColumnWidth(i, widths[i].toUInt());

	settings.beginGroup("notes");
	auto groups = settings.childGroups();
	for(auto &group:groups)
	{
		settings.beginGroup(group);
		auto &newNote = notes.emplace_back(std::make_unique<Note>());
		newNote->name = settings.value("name").toString();;
		newNote->content.code = settings.value("content").toString();
		newNote->activeNotify = settings.value("activeNotify").toBool();
		newNote->dtNotify = settings.value("notification").toDateTime();
		newNote->dtPostpone = settings.value("notifyPostpone").toDateTime();
		settings.endGroup();
	}
	settings.endGroup();
}

int MainWidget::RowOfNote(Note * note)
{
	for(uint i=0; i<notes.size(); i++)
		if(note == notes[i].get()) return i;
	if(note) QMbError("RowOfNote: ROW NOT FOUND for note " + note->name + " ("+note->dtNotify.toString()+")");
	else QMbError("RowOfNote: nullptr row");
	return -1;
}

namespace ColIndexes {
	const int name = 0;
	const int chBox = name+1;
	const int notifyDTedit = chBox+1;
	const int postponeDTedit = notifyDTedit+1;
}

void MainWidget::MakeNewRowInMainTable(Note * newNote)
{
	newNote->ConnectUpdated([this, newNote](){ UpdateRowFromNote(newNote, RowOfNote(newNote)); });

	table->setRowCount(table->rowCount()+1);

	int rowIndex = table->rowCount()-1;

	table->setItem(rowIndex, ColIndexes::name, new QTableWidgetItem);

	auto chActive = new QCheckBox;
	connect(chActive, &QCheckBox::stateChanged, [newNote, chActive](int){
		newNote->activeNotify = chActive->isChecked();
	});
	auto wCh = new QWidget;
	auto loWCH = new QHBoxLayout(wCh);
	loWCH->setAlignment(Qt::AlignCenter);
	loWCH->setContentsMargins(0,0,0,0);
	loWCH->addWidget(chActive);
	table->setCellWidget(rowIndex, ColIndexes::chBox, wCh);

	auto dtEditNotify = new QDateTimeEdit;
	dtEditNotify->setDisplayFormat("dd.MM.yyyy HH:mm");
	dtEditNotify->setCalendarPopup(true);
	table->setCellWidget(rowIndex, ColIndexes::notifyDTedit, dtEditNotify);

	auto dtEditPostpone = new QDateTimeEdit;
	dtEditNotify->setDisplayFormat("dd.MM.yyyy HH:mm");
	dtEditNotify->setCalendarPopup(true);
	table->setCellWidget(rowIndex, ColIndexes::postponeDTedit, dtEditPostpone);

	rowViews.emplace_back(table->item(rowIndex, ColIndexes::name), chActive, dtEditNotify, dtEditPostpone);

	UpdateRowFromNote(newNote, rowIndex);

	connect(dtEditNotify, &QDateTimeEdit::dateTimeChanged, [newNote, dtEditPostpone](const QDateTime &datetime){
		newNote->dtNotify = datetime;
		dtEditPostpone->setDateTime(datetime);
	});
	connect(dtEditPostpone, &QDateTimeEdit::dateTimeChanged, [newNote](const QDateTime &datetime){
		newNote->dtPostpone = datetime;
	});
}

void MainWidget::UpdateRowFromNote(Note * note, int row)
{
	rowViews[row].item->setText(note->name);
	rowViews[row].chBox->setChecked(note->activeNotify);
	rowViews[row].dteNotify->setDateTime(note->dtNotify);
	rowViews[row].dtePostpone->setDateTime(note->dtPostpone);
}
