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
		if(notes[i].note->index != i)
		{
			notes[i].note->index = i;
			notes[i].note->SaveNote();
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
	auto crNewNoteFoo = [this](){
		QString newName = MyQDialogs::InputLine("Создание заметки", "Введите название заметки", "").text;
		if(newName.isEmpty()) return;

		auto dt = QDateTime::currentDateTime();
		auto &newNote = MakeNewNote(newName, false, dt, dt.addSecs(3600), Note::StartText());
		UpdateNotesIndexes();

		WidgetNoteEditor::MakeOrShowNoteEditor(newNote, true);
	};
	hlo1->addWidget(btnPlus);
	connect(btnPlus,&QPushButton::clicked, crNewNoteFoo);

	QPushButton *btnRemove = new QPushButton("-");
	btnRemove->setFixedWidth(25);
	hlo1->addWidget(btnRemove);
	connect(btnRemove,&QPushButton::clicked,[this](){
		if(QMessageBox::question(0,"Remove note","Are you shure?") == QMessageBox::Yes)
		{
			if(auto note = NoteOfRow(table->currentRow()); note)
				note->Remove();
			else QMbError("NoteOfRow(table->currentRow()) returned nullptr");
		}
	});

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
		WidgetNoteEditor::MakeOrShowNoteEditor(*notes[r].note.get());
	});

	QDir().mkpath(Note::notesSavesPath);

	auto labelToGetFont = new QLabel("labelToGetFont");
	vlo_main->addWidget(labelToGetFont);

	QTimer::singleShot(0,[this, labelToGetFont, crNewNoteFoo]{

		auto showMainWindow = [this](){
			this->showNormal();
			PlatformDependent::SetTopMostFlash(this);
		};

		widgetAlarms = std::make_unique<WidgetAlarms>(labelToGetFont->font(), crNewNoteFoo, showMainWindow);
		delete labelToGetFont;

		LoadSettings();
		LoadNotes();
	});

	QTimer::singleShot(200,[this]{ FitColWidth(); });

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
		if(note.note->CheckAlarm(currentDateTime))
		{
			alarmedNotes.emplace_back(note.note.get());
		}
	}

	widgetAlarms->GiveNotes(alarmedNotes);
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
	{
		if(note == notes[i].note.get())
		{
			int row = table->row(notes[i].rowView.item);
			if(row == -1) QMbError("RowOfNote: -1");
			return row;
		}
	}
	if(note) QMbError("RowOfNote: ROW NOT FOUND for note " + note->name + " ("+note->DTNotify().toString()+")");
	else QMbError("RowOfNote: nullptr row");
	return -1;
}

Note *WidgetMain::NoteOfRow(int row)
{
	auto item = table->item(row, ColIndexes::name);
	if(!item) { QMbError("NoteOfRow: !item for row " + QSn(row)); return nullptr; }

	for(uint i=0; i<notes.size(); i++)
	{
		if(notes[i].rowView.item == item) return notes[i].note.get();
	}
	QMbError("NoteOfRow: NOTE NOT FOUND for row("+QSn(row)+") and existing item " + item->text());
	return nullptr;
}

Note & WidgetMain::MakeNewNote(QString name, bool activeNotify, QDateTime dtNotify, QDateTime dtPostpone, QString content)
{
	NoteInMain &newNoteInMainRef = notes.emplace_back();
	newNoteInMainRef.note = std::make_unique<Note>();
	Note* newNote = newNoteInMainRef.note.get();
	newNote->name = std::move(name);
	newNote->activeNotify = activeNotify;
	newNote->SetDT(dtNotify, dtPostpone);
	newNote->content.code = content;

	int newNoteIndex = MakeNewRowInMainTable(newNoteInMainRef);

	newNote->index = newNoteIndex;

	newNote->SaveNote();

	newNote->ConnectCommonUpdated([newNote](){ newNote->SaveNote(); });

	newNote->removeWorker = [this, newNote](){
		RemoveNote(newNote);
		CheckNotesForAlarm();
	};

	return *newNote;
}

int WidgetMain::MakeNewRowInMainTable(NoteInMain &newNote)
{
	newNote.note->ConnectCommonUpdated([this, &newNote](){ UpdateRowFromNote(newNote); });

	table->setRowCount(table->rowCount()+1);

	int rowIndex = table->rowCount()-1;

	table->setItem(rowIndex, ColIndexes::name, new QTableWidgetItem);

	auto chActive = new QCheckBox;
	connect(chActive, &QCheckBox::stateChanged, [&newNote, chActive](int){
		newNote.note->activeNotify = chActive->isChecked();
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

	newNote.rowView = RowView(table->item(rowIndex, ColIndexes::name), chActive, dtEditNotify, dtEditPostpone);

	UpdateRowFromNote(newNote);

	connect(dtEditNotify, &QDateTimeEdit::dateTimeChanged, [&newNote, dtEditPostpone](const QDateTime &datetime){
		newNote.note->SetDT(datetime, datetime);

		dtEditPostpone->blockSignals(true);
		dtEditPostpone->setDateTime(datetime);
		dtEditPostpone->blockSignals(false);

		newNote.note->SaveNote();
	});
	connect(dtEditPostpone, &QDateTimeEdit::dateTimeChanged, [&newNote](const QDateTime &datetime){
		newNote.note->SetDT(newNote.note->DTNotify(), datetime);
		newNote.note->SaveNote();
	});

	return rowIndex;
}

void WidgetMain::UpdateRowFromNote(NoteInMain &note)
{
	note.rowView.item->setText("   " + note.note->name);
	note.rowView.chBox->setChecked(note.note->activeNotify);
	note.rowView.dteNotify->setDateTime(note.note->DTNotify());
	note.rowView.dtePostpone->setDateTime(note.note->DTPostpone());
}

void WidgetMain::RemoveNote(Note* note)
{
	for(uint index=0; index<notes.size(); index++)
	{
		auto &notePl = notes[index];
		if(notePl.note.get() == note)
		{
			if(!note->file.isEmpty())
			{
				if(!QFile::remove(note->file)) QMbError("Error removing file " + note->file);
			}
			else QMbError("note("+note->name+")->file is empty");

			if(int row = RowOfNote(note); row != -1)
			{
				table->removeRow(row);
			}

			notes.erase(notes.begin() + index);

			UpdateNotesIndexes();
			return;
		}
	}
	if(!note) QMbError("RemoveNote nullptr note get");
	else QMbError("note("+note->name+") not found");
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

	int nameWidth = table->width() - (ColIndexes::chBoxWidth+ColIndexes::notifyDTeditWidth+ColIndexes::postponeDTeditWidth + 5);
	if(table->verticalScrollBar()->isVisible()) nameWidth -= table->verticalScrollBar()->width();
	table->setColumnWidth(ColIndexes::name, nameWidth);
}

void WidgetMain::resizeEvent(QResizeEvent * event)
{
	QWidget::resizeEvent(event);
	FitColWidth();
}
