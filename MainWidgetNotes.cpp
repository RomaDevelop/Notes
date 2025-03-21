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
#include <QScrollBar>

#include "MyQDifferent.h"
#include "MyQDialogs.h"
#include "MyQFileDir.h"
#include "MyQExecute.h"
#include "MyQWidgetLib.h"
#include "MyQTableWidget.h"
#include "CodeMarkers.h"

#include "NoteEditor.h"

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


MainWidget::MainWidget(QWidget *parent) : QWidget(parent)
{
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
	});

	QPushButton *btnRemove = new QPushButton("-");
	btnRemove->setFixedWidth(25);
	hlo1->addWidget(btnRemove);
	connect(btnRemove,&QPushButton::clicked,[this](){
		int index = table->currentRow();
		if(table->rowCount() == 0 ||  index < 0 || index >= table->rowCount()) return;
		if(!notes[index]->file.isEmpty())
		{
			if(!QFile::remove(notes[index]->file)) QMbError("Error removing file " + notes[index]->file);
		}
		notes.erase(notes.begin() + index);

		rowViews.erase(rowViews.begin() + index);
		table->removeRow(index);
	});

	QPushButton *btnRename = new QPushButton("Rename");
	btnRename->setFixedWidth(QFontMetrics(btnRename->font()).horizontalAdvance(btnRename->text()) + 20);
	hlo1->addWidget(btnRename);
	connect(btnRename,&QPushButton::clicked,[this](){
		int index = table->currentRow();
		if(table->rowCount() == 0 ||  index < 0 || index >= table->rowCount()) return;
		notes[index]->name = MyQDialogs::InputText("Измените название заметки", notes[index]->name);
		notes[index]->file = MakeNameFileToSaveNote(notes[index].get(), index);
		WriteNoteToFile(notes[index].get(), notes[index]->file);
		if(!notes[index]->file.isEmpty())
		{
			if(!QFile::remove(notes[index]->file)) QMbError("Error removing file " + notes[index]->file);
			notes[index]->file.clear();
		}
		table->item(index, 0)->setText(notes[index]->name);
	});

	hlo1->addStretch();

	QPushButton *btnSavePath = new QPushButton("Sava path");
	btnSavePath->setFixedWidth(QFontMetrics(btnSavePath->font()).horizontalAdvance(btnSavePath->text()) + 20);
	hlo1->addWidget(btnSavePath);
	connect(btnSavePath,&QPushButton::clicked,[this](){
		MyQExecute::OpenDir(notesSavesPath);
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
		NoteEditor::MakeNoteEditor(*notes[r].get());
	});

	QDir().mkpath(notesSavesPath);
	MyQFileDir::RemoveOldFiles(notesSavesPath, 30);

	QTimer::singleShot(0,this,[this]{ LoadSettings(); });

	QTimer::singleShot(200,this,[this]{ FitColWidth(); });

	CreateTrayIcon();
	CreateNotesChecker();
}

MainWidget::~MainWidget()
{
	/// не надо тут пытаться сохранять задачи
	/// ибо при завершении работы программы инициированном ОС они не будут успевать сохраняться
	/// сохранение задач должно надежно происходить при их изменении
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
			if(note->CheckAlarm(currentDateTime))
			{
				alarmedNotes.emplace_back(note.get());
			}
		}

		widgetAlarms.GiveNotes(alarmedNotes);
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


	/// не надо тут пытаться сохранять задачи
	/// ибо при завершении работы программы инициированном ОС они не будут успевать сохраняться
	/// сохранение задач должно надежно происходить при их изменении

	SaveSettings();
	event->accept();
	QApplication::exit();
}

QString MainWidget::MakeNameFileToSaveNote(Note * note, int index)
{
	QString fileName = notesSavesPath;
	fileName += "/note";
	fileName += QSn(index).rightJustified(5,'0');
	fileName += "-";
	fileName += QString(note->name).replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");
	fileName += ".txt";
	return fileName;
}

void MainWidget::WriteNoteToFile(Note * note, const QString & fileName)
{
	QString endValue = "[endValue]\n";
	QString dtFormat = "yyyy.MM.dd hh:mm:ss";

	QString noteText;
	noteText.append(note->name + endValue);
	noteText.append(QSn(note->activeNotify) + endValue);
	noteText.append(note->dtNotify.toString(dtFormat) + endValue);
	noteText.append(note->dtPostpone.toString(dtFormat) + endValue);
	noteText.append(note->content.code + endValue);
	if(!MyQFileDir::WriteFile(fileName, noteText))
	{
		QMbError("Во время сохранения произошла ошибка, сейчас будет показан файл который не сохранился, сохраните содержимое самостоятельно");
		MyQDialogs::ShowText(noteText);
	}
}

void MainWidget::SaveSettings()
{
	MyQFileDir::WriteFile(settingsFile, "");
	QSettings settings(settingsFile, QSettings::IniFormat);

	/// не надо тут пытаться сохранять задачи
	/// ибо при завершении работы программы инициированном ОС они не будут успевать сохраняться
	/// сохранение задач должно надежно происходить при их изменении

	settings.setValue("geoMainWidget", this->saveGeometry());
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

	QString endValue = "[endValue]\n";
	QString dtFormat = "yyyy.MM.dd hh:mm:ss";
	auto files = QDir(notesSavesPath).entryList(QDir::Files,QDir::Name);
	for(auto &file:files)
	{
		auto fileContent = MyQFileDir::ReadFile1(notesSavesPath + "/" + file);
		auto fileParts = fileContent.split(endValue, QString::SkipEmptyParts);
		if(fileParts.size() != 5) { QMbError("Wrong LoadSettings file " + notesSavesPath + "/" + file); continue; }

		MakeNewNote(fileParts[0], fileParts[1].toInt(),
				QDateTime().fromString(fileParts[2], dtFormat),
				QDateTime().fromString(fileParts[3], dtFormat),
				fileParts[4]);
	}
}

int MainWidget::RowOfNote(Note * note)
{
	for(uint i=0; i<notes.size(); i++)
		if(note == notes[i].get()) return i;
	if(note) QMbError("RowOfNote: ROW NOT FOUND for note " + note->name + " ("+note->dtNotify.toString()+")");
	else QMbError("RowOfNote: nullptr row");
	return -1;
}

Note & MainWidget::MakeNewNote(QString name, bool activeNotify, QDateTime dtNotify, QDateTime dtPostpone, QString content)
{
	notes.emplace_back(std::make_unique<Note>());
	Note* newNote = notes.back().get();
	newNote->name = std::move(name);
	newNote->file = MakeNameFileToSaveNote(newNote, notes.size()-1);
	newNote->activeNotify = activeNotify;
	newNote->dtNotify = dtNotify;
	newNote->dtPostpone = dtPostpone;
	newNote->content.code = content;

	WriteNoteToFile(newNote, newNote->file);

	newNote->ConnectContentUpdated([this, newNote](){ WriteNoteToFile(newNote, newNote->file); });

	MakeNewRowInMainTable(newNote);

	return *newNote;
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
	dtEditNotify->setDisplayFormat("dd.MM.yyyy HH:mm:ss");
	dtEditNotify->setCalendarPopup(true);
	table->setCellWidget(rowIndex, ColIndexes::notifyDTedit, dtEditNotify);

	auto dtEditPostpone = new QDateTimeEdit;
	dtEditPostpone->setDisplayFormat("dd.MM.yyyy HH:mm:ss");
	table->setCellWidget(rowIndex, ColIndexes::postponeDTedit, dtEditPostpone);

	rowViews.emplace_back(table->item(rowIndex, ColIndexes::name), chActive, dtEditNotify, dtEditPostpone);

	UpdateRowFromNote(newNote, rowIndex);

	connect(dtEditNotify, &QDateTimeEdit::dateTimeChanged, [this, newNote, dtEditPostpone](const QDateTime &datetime){
		newNote->dtNotify = datetime;
		newNote->dtPostpone = datetime;

		dtEditPostpone->blockSignals(true);
		dtEditPostpone->setDateTime(datetime);
		dtEditPostpone->blockSignals(false);

		WriteNoteToFile(newNote, newNote->file);
	});
	connect(dtEditPostpone, &QDateTimeEdit::dateTimeChanged, [this, newNote](const QDateTime &datetime){
		newNote->dtPostpone = datetime;

		WriteNoteToFile(newNote, newNote->file);
	});
}

void MainWidget::UpdateRowFromNote(Note * note, int row)
{
	rowViews[row].item->setText("   " + note->name);
	rowViews[row].chBox->setChecked(note->activeNotify);
	rowViews[row].dteNotify->setDateTime(note->dtNotify);
	rowViews[row].dtePostpone->setDateTime(note->dtPostpone);
}

void MainWidget::FitColWidth()
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

void MainWidget::resizeEvent(QResizeEvent * event)
{
	QWidget::resizeEvent(event);
	FitColWidth();
}
