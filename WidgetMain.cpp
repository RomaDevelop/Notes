#include "WidgetMain.h"

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
#include <QScrollBar>

#include "MyQDifferent.h"
#include "MyQDialogs.h"
#include "MyQFileDir.h"
#include "MyQExecute.h"
#include "PlatformDependent.h"
#include "MyQTableWidget.h"
#include "CodeMarkers.h"

#include "WidgetNoteEditor.h"

namespace ColIndexes {
	const int name = 0;
	const int chBox = name+1;
	const int notifyDTedit = chBox+1;
	const int postponeDTedit = notifyDTedit+1;

	const int colsCount = postponeDTedit+1;

	//const int nameWidth = -1;
	const int chBoxWidth = 60;
	const int notifyDTeditWidth = 130;
	const int postponeDTeditWidth = 130;
}

void WidgetMain::UpdateNotesIndexes()
{
	if(0) CodeMarkers::to_do("do not resave all note file for change index");
	for(int i=0; i<(int)notes.size(); i++)
	{
		if(notes[i]->index != i)
		{
			notes[i]->index = i;
			notes[i]->SaveNote();
		}
	}
}

WidgetMain::WidgetMain(QWidget *parent) : QWidget(parent)
{
	Note::notesSavesPath = filesPath + "/notes";
	Note::notesBackupsPath = filesPath + "/notes_backups";

	QVBoxLayout *vlo_main = new QVBoxLayout(this);
	QHBoxLayout *hlo1 = new QHBoxLayout;
	QHBoxLayout *hlo2 = new QHBoxLayout;
	vlo_main->addLayout(hlo1);
	vlo_main->addLayout(hlo2);

	QPushButton *btnPlus = new QPushButton("+");
	btnPlus->setFixedWidth(25);
	hlo1->addWidget(btnPlus);
	connect(btnPlus,&QPushButton::clicked,[this](){
		QString newName = MyQDialogs::InputText("Введите название заметки", "", 400, 150);
		if(newName.isEmpty()) return;

		auto dt = QDateTime::currentDateTime();
		MakeNewNote(newName, false, dt, dt, Note::StartText());
		UpdateNotesIndexes();
	});

	QPushButton *btnRemove = new QPushButton("-");
	btnRemove->setFixedWidth(25);
	hlo1->addWidget(btnRemove);
	connect(btnRemove,&QPushButton::clicked,[this](){ RemoveNote(table->currentRow()); });

	hlo1->addStretch();

	QPushButton *btnSavePath = new QPushButton("Save path");
	btnSavePath->setFixedWidth(QFontMetrics(btnSavePath->font()).horizontalAdvance(btnSavePath->text()) + 20);
	hlo1->addWidget(btnSavePath);
	connect(btnSavePath,&QPushButton::clicked,[](){
		MyQExecute::OpenDir(Note::notesSavesPath);
	});

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
	table->verticalHeader()->hide();
	table->setColumnCount(4);
	table->setHorizontalHeaderLabels({"Наименование","","Начало","Отложено на..."});
	hlo2->addWidget(table);
	connect(table, &QTableWidget::cellDoubleClicked, [this](int r, int){
		WidgetNoteEditor::MakeNoteEditor(*notes[r].get());
	});

	QDir().mkpath(Note::notesSavesPath);

	QTimer::singleShot(0,this,[this]{
		LoadSettings();
		LoadNotes();
	});

	QTimer::singleShot(200,this,[this]{ FitColWidth(); });

	CreateTrayIcon();
	CreateNotesAlarmChecker();
}

WidgetMain::~WidgetMain()
{
	/// не надо тут пытаться сохранять задачи
	/// ибо при завершении работы программы инициированном ОС они не будут успевать сохраняться
	/// сохранение задач должно надежно происходить при их изменении
}

void WidgetMain::CreateTrayIcon()
{
	auto icon = new QSystemTrayIcon(this);
	icon->setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowForward));
	icon->setToolTip("Notes");
	icon->show();
	connect(icon, &QSystemTrayIcon::activated, [this](){
		showNormal();
		PlatformDependent::SetTopMost(this,true);
		PlatformDependent::SetTopMost(this,false);
	});
}

void WidgetMain::CreateNotesAlarmChecker()
{
	QTimer *tChecher = new QTimer(this);
	connect(tChecher, &QTimer::timeout, [this](){ CheckNotesForAlarm(); });
	tChecher->start(1000);
}

void WidgetMain::CheckNotesForAlarm()
{
	if(0) CodeMarkers::to_do("Нужно адекватный алгоритм проверки чтобы не ломалось если будет много задач");
	QDateTime currentDateTime = QDateTime::currentDateTime();
	std::vector<Note*> alarmedNotes;
	for(auto &note:notes)
	{
		if(note->CheckAlarm(currentDateTime))
		{
			alarmedNotes.emplace_back(note.get());
		}
	}

	widgetAlarms.GiveNotes(alarmedNotes);
}

void WidgetMain::closeEvent(QCloseEvent * event)
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


	/// не надо тут пытаться сохранять задачи
	/// ибо при завершении работы программы инициированном ОС они не будут успевать сохраняться
	/// сохранение задач должно надежно происходить при их изменении

	SaveSettings();
	event->accept();
	QApplication::exit();
}

void WidgetMain::SaveSettings()
{
	MyQFileDir::WriteFile(settingsFile, "");
	QSettings settings(settingsFile, QSettings::IniFormat);

	/// не надо тут пытаться сохранять задачи
	/// ибо при завершении работы программы инициированном ОС они не будут успевать сохраняться
	/// сохранение задач должно надежно происходить при их изменении

	settings.setValue("geoMainWidget", this->saveGeometry());
}

void WidgetMain::LoadSettings()
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
}

void WidgetMain::LoadNotes()
{
	if(!QDir().mkpath(Note::notesBackupsPath)) QMbError("mkpath error " + Note::notesBackupsPath);
	MyQFileDir::RemoveOldFiles(Note::notesBackupsPath, 1000);

	QString endValue = "[endValue]\n";
	QString dtFormat = "yyyy.MM.dd hh:mm:ss";
	QString currentDt = QDateTime::currentDateTime().toString(DateTimeFormatForFileName);
	auto files = QDir(Note::notesSavesPath).entryList(QDir::Files,QDir::Name);
	for(auto &file:files)
	{
		QString filePathName = Note::notesSavesPath + "/" + file;
		if(!QFile::copy(filePathName, Note::notesBackupsPath + "/" + currentDt + " " + file))
			QMbError("creation backup note error for file  " + file);

		auto fileContent = MyQFileDir::ReadFile1(filePathName);
		auto fileParts = fileContent.split(endValue, QString::SkipEmptyParts);
		if(fileParts.size() != 5) { QMbError("Wrong LoadSettings file " + Note::notesSavesPath + "/" + file); continue; }

		MakeNewNote(fileParts[0], fileParts[1].toInt(),
				QDateTime().fromString(fileParts[2], dtFormat),
				QDateTime().fromString(fileParts[3], dtFormat),
				fileParts[4]);
	}
}

int WidgetMain::RowOfNote(Note * note)
{
	for(uint i=0; i<notes.size(); i++)
		if(note == notes[i].get()) return i;
	if(note) QMbError("RowOfNote: ROW NOT FOUND for note " + note->name + " ("+note->dtNotify.toString()+")");
	else QMbError("RowOfNote: nullptr row");
	return -1;
}

Note & WidgetMain::MakeNewNote(QString name, bool activeNotify, QDateTime dtNotify, QDateTime dtPostpone, QString content)
{
	notes.emplace_back(std::make_unique<Note>());
	Note* newNote = notes.back().get();
	newNote->name = std::move(name);
	newNote->activeNotify = activeNotify;
	newNote->dtNotify = dtNotify;
	newNote->dtPostpone = dtPostpone;
	newNote->content.code = content;

	int newNoteIndex = MakeNewRowInMainTable(newNote);

	newNote->index = newNoteIndex;

	newNote->SaveNote();

	newNote->ConnectUpdated([newNote](){ newNote->SaveNote(); });

	newNote->removeWorker = [this, newNote](){
		RemoveNote(RowOfNote(newNote));
		CheckNotesForAlarm();
	};

	return *newNote;
}

int WidgetMain::MakeNewRowInMainTable(Note * newNote)
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
	dtEditNotify->setDisplayFormat("dd.MM.yyyy HH:mm:ss");
	dtEditNotify->setCalendarPopup(true);
	table->setCellWidget(rowIndex, ColIndexes::notifyDTedit, dtEditNotify);

	auto dtEditPostpone = new QDateTimeEdit;
	dtEditPostpone->setDisplayFormat("dd.MM.yyyy HH:mm:ss");
	table->setCellWidget(rowIndex, ColIndexes::postponeDTedit, dtEditPostpone);

	rowViews.emplace_back(table->item(rowIndex, ColIndexes::name), chActive, dtEditNotify, dtEditPostpone);

	UpdateRowFromNote(newNote, rowIndex);

	connect(dtEditNotify, &QDateTimeEdit::dateTimeChanged, [newNote, dtEditPostpone](const QDateTime &datetime){
		newNote->dtNotify = datetime;
		newNote->dtPostpone = datetime;

		dtEditPostpone->blockSignals(true);
		dtEditPostpone->setDateTime(datetime);
		dtEditPostpone->blockSignals(false);

		newNote->SaveNote();
	});
	connect(dtEditPostpone, &QDateTimeEdit::dateTimeChanged, [newNote](const QDateTime &datetime){
		newNote->dtPostpone = datetime;
		newNote->SaveNote();
	});

	return rowIndex;
}

void WidgetMain::UpdateRowFromNote(Note * note, int row)
{
	rowViews[row].item->setText("   " + note->name);
	rowViews[row].chBox->setChecked(note->activeNotify);
	rowViews[row].dteNotify->setDateTime(note->dtNotify);
	rowViews[row].dtePostpone->setDateTime(note->dtPostpone);
}

void WidgetMain::RemoveNote(int index)
{
	if(table->rowCount() == 0 ||  index < 0 || index >= table->rowCount()) return;
	if(!notes[index]->file.isEmpty())
	{
		if(!QFile::remove(notes[index]->file)) QMbError("Error removing file " + notes[index]->file);
	}
	notes.erase(notes.begin() + index);
	UpdateNotesIndexes();

	rowViews.erase(rowViews.begin() + index);
	table->removeRow(index);
}

void WidgetMain::FitColWidth()
{
	int columnCount = table->columnCount();

	if (columnCount != ColIndexes::colsCount)
	{
		static bool preinted = false;
		if(!preinted) { preinted = true; QMbError("resizeEvent wrong columnCount"); }
		return;
	}

	table->setColumnWidth(ColIndexes::chBox, ColIndexes::chBoxWidth);
	table->setColumnWidth(ColIndexes::notifyDTedit, ColIndexes::notifyDTeditWidth);
	table->setColumnWidth(ColIndexes::postponeDTedit, ColIndexes::postponeDTeditWidth);

	int nameWidth = table->width() - (ColIndexes::chBoxWidth+ColIndexes::notifyDTeditWidth+ColIndexes::postponeDTeditWidth);
	if(table->verticalScrollBar()->isVisible()) nameWidth -= table->verticalScrollBar()->width() + 5;
	table->setColumnWidth(ColIndexes::name, nameWidth);
}

void WidgetMain::resizeEvent(QResizeEvent * event)
{
	QWidget::resizeEvent(event);
	FitColWidth();
}
