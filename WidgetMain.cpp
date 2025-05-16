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
#include <QToolButton>
#include <QTextEdit>
#include <QDateTimeEdit>
#include <QSystemTrayIcon>
#include <QApplication>
#include <QScrollBar>

#include "MyQDifferent.h"
#include "MyQString.h"
#include "MyQDialogs.h"
#include "MyQFileDir.h"
#include "MyQExecute.h"
#include "MyQWidget.h"
#include "PlatformDependent.h"
#include "MyQTableWidget.h"
#include "CodeMarkers.h"
#include "Resources.h"

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
		if(notes[i]->note->index != i)
		{
			notes[i]->note->index = i;
			notes[i]->note->SaveNote();
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

// HeaderPanel
	CreateHeaderPanel(hlo1);

// Table
	table = new QTableWidget;
	table->verticalHeader()->hide();
	table->setColumnCount(4);
	table->setHorizontalHeaderLabels({"Наименование","","Начало","Отложено на..."});
	hlo2->addWidget(table);
	connect(table, &QTableWidget::cellDoubleClicked, [this](int r, int){
		WidgetNoteEditor::MakeOrShowNoteEditor(*notes[r]->note.get());
	});

	QDir().mkpath(Note::notesSavesPath);

	auto labelToGetFont = new QLabel("labelToGetFont");
	vlo_main->addWidget(labelToGetFont);

	QTimer::singleShot(0,[this, labelToGetFont]{

		auto showMainWindow = [this](){
			this->showNormal();
			PlatformDependent::SetTopMostFlash(this);
		};

		widgetAlarms = std::make_unique<WidgetAlarms>(labelToGetFont->font(), [this](){ SlotCreationNewNote(); }, showMainWindow);
		delete labelToGetFont;

		LoadSettings();
		LoadNotes();
	});

	CreateTrayIcon();
	CreateNotesAlarmChecker();
}

WidgetMain::~WidgetMain()
{
	/// не надо тут пытаться сохранять задачи
	/// ибо при завершении работы программы инициированном ОС они не будут успевать сохраняться
	/// сохранение задач должно надежно происходить при их изменении
}

void WidgetMain::CreateHeaderPanel(QHBoxLayout *hlo1)
{
	QToolButton *btnPlus = new QToolButton();
	btnPlus->setIcon(QIcon(Resources::add().GetPathName()));
	hlo1->addWidget(btnPlus);
	connect(btnPlus,&QPushButton::clicked, this, &WidgetMain::SlotCreationNewNote);

	QToolButton *btnRemove = new QToolButton();
	btnRemove->setIcon(QIcon(Resources::remove().GetPathName()));
	hlo1->addWidget(btnRemove);
	connect(btnRemove,&QPushButton::clicked,[this](){
		auto note = NoteOfRow(table->currentRow());
		if(!note) { QMbError("NoteOfRow(table->currentRow()) returned nullptr"); return; }

		if(QMessageBox::question(0,"Remove note","Removing note "+note->Name()+"\n\nAre you shure?") == QMessageBox::Yes)
				note->RemoveNoteFromBase();
	});

	auto btnFastActions = new QToolButton();
	btnFastActions->setIcon(QIcon(Resources::action().GetPathName()));
	hlo1->addWidget(btnFastActions);
	connect(btnFastActions, &QPushButton::clicked, [this, btnFastActions](){
		if(table->currentRow() == -1) return;
		if(auto note = NoteOfRow(table->currentRow()); note)
			note->ShowDialogFastActions(btnFastActions);
		else QMbError("NoteOfRow(table->currentRow()) returned nullptr");
	});

// Search
	hlo1->addSpacing(20);
	auto hloSearch = new QHBoxLayout;
	hloSearch->setContentsMargins(0,0,0,0);
	hloSearch->setSpacing(0);
	hlo1->addLayout(hloSearch);

	auto leSearch = new QLineEdit;
	hloSearch->addWidget(leSearch);
	connect(leSearch, &QLineEdit::textChanged, [this](const QString &text){ FilterNotes(text); });
	auto btnClearSearch = new QToolButton;
	btnClearSearch->setIcon(QApplication::style()->standardIcon(QStyle::StandardPixmap::SP_TitleBarCloseButton));
	hloSearch->addWidget(btnClearSearch);
	connect(btnClearSearch, &QToolButton::clicked, [leSearch](){ leSearch->clear(); });

	hlo1->addSpacing(20);

// Save path
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
}

void WidgetMain::CreateTrayIcon()
{
	auto icon = new QSystemTrayIcon(this);
	icon->setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowForward));
	icon->show();

	QString toolTip = "Notes";
#ifdef QT_DEBUG
	toolTip += " debug";
#endif
	icon->setToolTip(toolTip);

	QMenu *menu = new QMenu(this);
	icon->setContextMenu(menu);

	auto showFoo = [this](){
		showNormal();
		PlatformDependent::SetTopMost(this,true);
		PlatformDependent::SetTopMost(this,false);
	};

	connect(icon, &QSystemTrayIcon::activated, [icon, showFoo](QSystemTrayIcon::ActivationReason reason){
		if(reason == QSystemTrayIcon::Trigger) showFoo();
		if(reason == QSystemTrayIcon::Context) icon->contextMenu()->exec();
	});

	menu->addAction("Show main window");
	MyQWidget::SetFontBold(menu->actions().back(), true);
	connect(menu->actions().back(), &QAction::triggered, showFoo);

	menu->addAction("Hide main window");
	connect(menu->actions().back(), &QAction::triggered, this, &WidgetMain::hide);

	menu->addSeparator();

	menu->addAction("Create new note");
	connect(menu->actions().back(), &QAction::triggered, [this](){ SlotCreationNewNote(); });

	menu->addSeparator();

	menu->addAction("Close app");
	MyQWidget::SetFontBold(menu->actions().back(), true);
	connect(menu->actions().back(), &QAction::triggered, this, &QWidget::close);
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
		if(note->note->CheckAlarm(currentDateTime))
		{
			alarmedNotes.emplace_back(note->note.get());
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
	settings.setValue("tableHeaderState", this->table->horizontalHeader()->saveState());
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

	if(settings.contains("geoMainWidget")) restoreGeometry(settings.value("geoMainWidget").toByteArray());
	bool tableHeaderRestored = false;
	if(settings.contains("tableHeaderState"))
	{
		this->table->horizontalHeader()->restoreState(settings.value("tableHeaderState").toByteArray());
		tableHeaderRestored = true;
	}

	if(!tableHeaderRestored)
		QTimer::singleShot(200,[this]{ DefaultColsWidths(); });
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
		if(note == notes[i]->note.get())
		{
			int row = table->row(notes[i]->rowView.item);
			if(row == -1) QMbError("RowOfNote: -1");
			return row;
		}
	}
	if(note) QMbError("RowOfNote: ROW NOT FOUND for note " + note->Name() + " ("+note->DTNotify().toString()+")");
	else QMbError("RowOfNote: nullptr row");
	return -1;
}

Note *WidgetMain::NoteOfRow(int row)
{
	auto item = table->item(row, ColIndexes::name);
	if(!item) { QMbError("NoteOfRow: !item for row " + QSn(row)); return nullptr; }

	for(uint i=0; i<notes.size(); i++)
	{
		if(notes[i]->rowView.item == item) return notes[i]->note.get();
	}
	QMbError("NoteOfRow: NOTE NOT FOUND for row("+QSn(row)+") and existing item " + item->text());
	return nullptr;
}

void WidgetMain::SlotCreationNewNote()
{
	QString newName = MyQDialogs::InputLine("Создание заметки", "Введите название новой заметки", "").text;
	if(newName.isEmpty()) return;

	auto dt = QDateTime::currentDateTime();
	auto &newNote = MakeNewNote(newName, false, dt, dt.addSecs(3600), Note::StartText());
	UpdateNotesIndexes();

	WidgetNoteEditor::MakeOrShowNoteEditor(newNote, true);
}

Note & WidgetMain::MakeNewNote(QString name, bool activeNotify, QDateTime dtNotify, QDateTime dtPostpone, QString content)
{
	notes.emplace_back(std::unique_ptr<NoteInMain>(new NoteInMain));
	NoteInMain &newNoteInMainRef = *notes.back().get();
	newNoteInMainRef.note = std::make_unique<Note>();
	Note* newNote = newNoteInMainRef.note.get();
	newNote->SetName(std::move(name));
	newNote->activeNotify = activeNotify;
	newNote->SetDT(dtNotify, dtPostpone);
	newNote->SetContent(std::move(content));

	int newNoteIndex = MakeWidgetsForMainTable(newNoteInMainRef);

	newNote->index = newNoteIndex;

	newNote->SaveNote();

	auto saveNoteFoo = [newNote](void*){ newNote->SaveNote(); };

	newNote->SetCBNameUpdated(saveNoteFoo, &newNoteInMainRef, newNoteInMainRef.cbCounter);
	newNote->SetCBContentUpdated(saveNoteFoo, &newNoteInMainRef, newNoteInMainRef.cbCounter);
	newNote->SetCBDTUpdated(saveNoteFoo, &newNoteInMainRef, newNoteInMainRef.cbCounter);

	newNote->removeNoteFromBaseWorker = [this, newNote](){
		RemoveNote(newNote);
		CheckNotesForAlarm();
	};

	return *newNote;
}

int WidgetMain::MakeWidgetsForMainTable(NoteInMain &newNote)
{
	NoteInMain *newNotePtr = &newNote;
	auto updateRowFoo = [this, newNotePtr](void*){ UpdateWidgetsFromNote(*newNotePtr); };

	newNote.note->SetCBNameUpdated(updateRowFoo, &newNote, newNote.cbCounter);
	newNote.note->SetCBDTUpdated(updateRowFoo, &newNote, newNote.cbCounter);

	auto item = new QTableWidgetItem;

	auto chActive = new QCheckBox;
	connect(chActive, &QCheckBox::stateChanged, [&newNote, chActive](int){
		newNote.note->activeNotify = chActive->isChecked();
	});

	// сигнал удаления виджетов
	//connect(chActive, &QObject::destroyed, [&newNote](){ qdbg << "widgets of " + newNote.note->Name() + " destroyed"; });

	auto widgetForChBox = new QWidget;
	auto loWCH = new QHBoxLayout(widgetForChBox);
	loWCH->setAlignment(Qt::AlignCenter);
	loWCH->setContentsMargins(0,0,0,0);
	loWCH->addWidget(chActive);

	auto dtEditNotify = new QDateTimeEdit;
	dtEditNotify->setDisplayFormat("dd.MM.yyyy HH:mm:ss");
	dtEditNotify->setCalendarPopup(true);

	auto dtEditPostpone = new QDateTimeEdit;
	dtEditPostpone->setDisplayFormat("dd.MM.yyyy HH:mm:ss");

	newNote.rowView = RowView(item, chActive, dtEditNotify, dtEditPostpone);

	UpdateWidgetsFromNote(newNote);

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

	table->setRowCount(table->rowCount()+1);
	int rowIndex = table->rowCount()-1;
	table->setItem(			rowIndex, ColIndexes::name, item);
	table->setCellWidget(	rowIndex, ColIndexes::chBox, widgetForChBox);
	table->setCellWidget(	rowIndex, ColIndexes::notifyDTedit, dtEditNotify);
	table->setCellWidget(	rowIndex, ColIndexes::postponeDTedit, dtEditPostpone);
	return rowIndex;
}

void WidgetMain::UpdateWidgetsFromNote(NoteInMain &note)
{
	note.rowView.item->setText("   " + note.note->Name());
	note.rowView.chBox->setChecked(note.note->activeNotify);
	note.rowView.dteNotify->setDateTime(note.note->DTNotify());
	note.rowView.dtePostpone->setDateTime(note.note->DTPostpone());
}

void WidgetMain::FilterNotes(const QString &nameFilter)
{
	for(int row=0; row<table->rowCount(); row++)
	{
		if(nameFilter.isEmpty()
				|| table->item(row, ColIndexes::name)->text().contains(nameFilter, Qt::CaseInsensitive)
				|| table->item(row, ColIndexes::name)->text().contains(MyQString::Translited(nameFilter), Qt::CaseInsensitive))
		{
			table->setRowHidden(row, false);
		}
		else
		{
			table->setRowHidden(row, true);
		}
	}
}

void WidgetMain::RemoveNote(Note* note)
{
	for(uint index=0; index<notes.size(); index++)
	{
		auto &notePl = notes[index];
		if(notePl->note.get() == note)
		{
			if(!note->file.isEmpty())
			{
				if(!QFile::remove(note->file)) QMbError("Error removing file " + note->file);
			}
			else QMbError("note("+note->Name()+")->file is empty");

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
	else QMbError("note("+note->Name()+") not found");
}

void WidgetMain::DefaultColsWidths()
{
	int columnCount = table->columnCount();

	if (columnCount != ColIndexes::colsCount)
	{
		static bool preinted = false;
		if(!preinted) { preinted = true; QMbError("FitColWidth wrong columnCount"); }
		return;
	}

	table->setColumnWidth(ColIndexes::chBox, ColIndexes::chBoxWidth);
	table->setColumnWidth(ColIndexes::notifyDTedit, ColIndexes::notifyDTeditWidth);
	table->setColumnWidth(ColIndexes::postponeDTedit, ColIndexes::postponeDTeditWidth);

	int nameWidth = table->width() - (ColIndexes::chBoxWidth+ColIndexes::notifyDTeditWidth+ColIndexes::postponeDTeditWidth + 5);
	if(table->verticalScrollBar()->isVisible()) nameWidth -= table->verticalScrollBar()->width();
	table->setColumnWidth(ColIndexes::name, nameWidth);
}
