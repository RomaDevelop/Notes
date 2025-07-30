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
#include <QTcpSocket>
#include <QCryptographicHash>
#include <QScreen>
#include <QFileDialog>

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
#include "LaunchParams.h"

#include "AdditionalTray.h"

#include "NetConstants.h"
#include "DataBase.h"
#include "WidgetNoteEditor.h"

void ToDo(){
	qdbg << "ToDo in WidgetMain.cpp line " + QSn(__LINE__);

//	#error

	/// (позже) создание заметки сразу в выбранной группе
	/// (позже) при измененнии заметки сервер отправляет всем остальным клиентам информацию
	/// (позже) делать историю редактирования заметок ибо возможна ситуация что на одном клиенте сохранили, а на сервер не ушло, 
	///				потом на другом клиенте сохранили и первые изменения будут потеряны
	///	(позже) для минимизации вышеописанной ситуации нужно сигнализировать о том что нет связи с сервером, 
	///				и еще хранить на клиенте неотгруженные изменения и если соединение появилось - отгружать их
	///	(позже) для сокращения трафика передавать сохрание не всей заметки, а по полям, а в случае контента - даже только измененный фрагмент
	///	(позже) сделать cb note removed и подключить к нему WidgetAlarms и в удалении убрать обязательное удальение строки из WidgetAlarms
	/// (позже) для сокращения трафика, расчитывать дату изменении группы заметок и сравнивать сначала её,
	///				а уже если надо - работать по заметкам группы
	///
	/// дебаг - работать локально, выпуск - через сервер
	///
	/// !!! Проход через проксю
	///
	/// После удаления заметки если она редактировалась, редактор должен закрываться и уничтожаться
	///
	/// Синхронизация заметок с сервером - в процессе
	///
	/// Развертка сервера
	/// 
	/// клиент и сервер сохраняют удалённые заметки каждый в своей корзине
	///
	/// запуск таймера из иконки в трее
	///
	/// нужна возможность хранить ссылки в заметках. Как?
}

namespace ColIndexes {
	const int name = 0;
	const int group = name+1;
	const int chBox = group+1;
	const int notifyDTedit = chBox+1;
	const int postponeDTedit = notifyDTedit+1;

	const int colsCount = postponeDTedit+1;

	inline const QStringList& colsCaptions() {
		static QStringList strList {"Наименование", "Группа", "", "Начало", "Отложено на..."}; return strList; }

	//const int nameWidth = -1;
	const int chBoxWidth = 60;
	const int notifyDTeditWidth = 130;
	const int postponeDTeditWidth = 130;
}

WidgetMain::WidgetMain(QWidget *parent) : QWidget(parent)
{
	if(!QDir().mkpath(filesPath)) QMbError("Error cration path " + filesPath);

	ToDo();

	auto base = DataBase::defineBase(DataBase::client);

#ifndef QT_DEBUG
	auto answ = MyQDialogs::CustomDialog("Launching Notes", "Do you want to update repo before launching Notes?",
										 {"Yes, update", "No update", "Abort launch"});
	if(answ == "No update") {}
	else if(answ == "Yes, update")
	{
		QString fetchRes;
		while(fetchRes != "finish")
		{
			for(int i=0; i<3; i++)
			{
				QProcess process;
				process.setWorkingDirectory(LaunchParams::CurrentDeveloper()->sourcesPath);
				process.start("git", QStringList() << "fetch" << "github");
				if(!process.waitForStarted(3000))
				{
					fetchRes = "error waitForStarted " + (QStringList() << "fetch" << "github").join(" ");
				}
				else if(!process.waitForFinished(3000))
				{
					fetchRes = "error waitForFinished " + (QStringList() << "fetch" << "github").join(" ");
				}
				else fetchRes = process.readAllStandardError();

				if(fetchRes.isEmpty()) break;
			}

			if(!fetchRes.isEmpty())
			{
				auto answ = QMessageBox::question(nullptr, "Fetch errors", "Fetch did with errors:\n\n"+fetchRes+"\n\nTry again?");
				if(answ == QMessageBox::No) fetchRes = "finish";
			}
			else fetchRes = "finish";
		}

		;
		if(GitExtensionsTool::ExecuteGitExtensions(base.pathDataBase, true, filesPath))
			QMbInfo("Launching GitExtensions...\n\nPress ok when you finish repo updating.");
		//else QMbError("Error launching GitExtensions"); // ExecuteGitExtensions выводит ошибку сам
	}
	else if(answ == "Abort launch")
	{
		this->deleteLater();
		if(0) CodeMarkers::to_do_afterwards("в этом сценарии вылетает крит, потому что при уничтожении окна идет обращение к виджетам, "
											"которые в этом сценарии еще не созданы");
		return;
	}
	else QMbError("Unexpacted answ");
#endif

	MyQSqlDatabase::Init(base, {},
							[](const QString &str){ qdbg << str; },
							[](const QString &str){ QMbError(str); });
	DataBase::InitChildDataBase(DataBase::client);
	DataBase::BackupBase();

	QString currentDt = QDateTime::currentDateTime().toString(DateTimeFormatForFileName);

	QVBoxLayout *vlo_main = new QVBoxLayout(this);
	QHBoxLayout *hlo1 = new QHBoxLayout;
	QHBoxLayout *hlo2 = new QHBoxLayout;
	vlo_main->addLayout(hlo1);
	vlo_main->addLayout(hlo2);

// Row1
	CreateRow1(hlo1);

// Table
	table = new QTableWidget;
	table->verticalHeader()->setVisible(false);
	table->setColumnCount(ColIndexes::colsCount);
	table->setHorizontalHeaderLabels(ColIndexes::colsCaptions());
	hlo2->addWidget(table);
	connect(table, &QTableWidget::cellDoubleClicked, [this](int r, int){
		WidgetNoteEditor::MakeOrShowNoteEditor(*notes[r]->note.get());
	});

	CreateTableContextMenu();

	auto labelToGetFont = new QLabel("labelToGetFont");
	vlo_main->addWidget(labelToGetFont);

	QTimer::singleShot(0,[this, labelToGetFont]{
		widgetAlarms = std::make_unique<WidgetAlarms>(this, labelToGetFont->font());
		delete labelToGetFont;

		LoadSettings();

		LoadGroupsSubscribes();

		LoadNotes();
	});

	QTimer *timerSettingsSaver = new QTimer(this);
	connect(timerSettingsSaver, &QTimer::timeout, [this](){ SaveSettings(); });
	timerSettingsSaver->start(1000*60);

	CreateTrayIcon();
	CreateNotesAlarmChecker();

	Note::InitTimerResaverNotSavedNotes(this);

	netClient = new NetClient(this);
	Note::netClient = netClient;
	connect(netClient, &NetClient::SignalNoteRemoved, this, &WidgetMain::SlotForNetClientNoteRemoved);
	connect(netClient, &NetClient::SignalNoteChangedGgroup, this, &WidgetMain::SlotForNetClientNoteChangedGroupOrUpdated);
	connect(netClient, &NetClient::SignalNoteUpdated, this, &WidgetMain::SlotForNetClientNoteChangedGroupOrUpdated);
	connect(netClient, &NetClient::SignalNewNoteAppeared, this, &WidgetMain::SlotForNetClientNewNoteAppeared);
}

WidgetMain::~WidgetMain()
{
	/// не надо тут пытаться сохранять задачи
	/// ибо при завершении работы программы инициированном ОС они не будут успевать сохраняться
	/// сохранение задач должно надежно происходить при их изменении
}

void WidgetMain::CreateRow1(QHBoxLayout *hlo1)
{
// Most opened notes
	auto btnMostOpenedNotes = new QToolButton();
	btnMostOpenedNotes->setIcon(QIcon(Resources::list_mo().GetPathName()));
	hlo1->addWidget(btnMostOpenedNotes);
	connect(btnMostOpenedNotes, &QPushButton::clicked, this, [this](){ MostOpenedNotes(); });

// Add
	QToolButton *btnPlus = new QToolButton();
	btnPlus->setIcon(QIcon(Resources::add().GetPathName()));
	hlo1->addWidget(btnPlus);
	connect(btnPlus,&QPushButton::clicked, this, [this](){ CreateNewNote(); });

// Remove
	QToolButton *btnRemove = new QToolButton();
	btnRemove->setIcon(QIcon(Resources::remove().GetPathName()));
	hlo1->addWidget(btnRemove);
	connect(btnRemove,&QPushButton::clicked,[this](){
		auto note = NoteOfRow(table->currentRow());
		if(!note) { QMbError("NoteOfRow(table->currentRow()) returned nullptr"); return; }

		if(QMessageBox::question(0,"Remove note","Removing note "+note->Name()+"\n\nAre you shure?") == QMessageBox::Yes)
				note->ExecRemoveNoteWorker();
	});

// FastActions
	auto btnFastActions = new QToolButton();
	btnFastActions->setIcon(QIcon(Resources::action().GetPathName()));
	hlo1->addWidget(btnFastActions);
	connect(btnFastActions, &QPushButton::clicked, [this, btnFastActions](){
		if(table->currentRow() == -1) return;
		if(auto note = NoteOfRow(table->currentRow()); note)
			note->ShowMenuFastActions(btnFastActions);
		else QMbError("NoteOfRow(table->currentRow()) returned nullptr");
	});

	hlo1->addSpacing(20);

// Menu
	QPushButton *btnMenu = new QPushButton("Menu");
	btnMenu->setFixedWidth(QFontMetrics(btnMenu->font()).horizontalAdvance(btnMenu->text()) + 20);
	hlo1->addWidget(btnMenu);
	connect(btnMenu, &QPushButton::clicked, [this, btnMenu]() { SlotMenu(btnMenu); });

	hlo1->addSpacing(20);

// Search
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

// Test
	QPushButton *btnTest = new QPushButton("Test");
	btnTest->setFixedWidth(QFontMetrics(btnTest->font()).horizontalAdvance(btnTest->text()) + 20);
	hlo1->addWidget(btnTest);
	connect(btnTest,&QPushButton::clicked, this, &WidgetMain::SlotTest);

// Synch
	QPushButton *btnSynch = new QPushButton("Synch");
	btnSynch->setFixedWidth(QFontMetrics(btnSynch->font()).horizontalAdvance(btnSynch->text()) + 20);
	hlo1->addWidget(btnSynch);
	connect(btnSynch, &QPushButton::clicked, [this](){
		netClient->request_all_notes_sending();
		//netClient->SynchronizeAllNotes(AllNotesVect());
	});

	QPushButton *btnToTray = new QPushButton("To tray");
	btnToTray->setFixedWidth(QFontMetrics(btnToTray->font()).horizontalAdvance(btnToTray->text()) + 20);
	hlo1->addWidget(btnToTray);
	connect(btnToTray,&QPushButton::clicked, this, &QWidget::hide);
}

void WidgetMain::CreateTableContextMenu()
{
	table->setContextMenuPolicy(Qt::CustomContextMenu);

	connect(table, &QWidget::customContextMenuRequested, [this](const QPoint& pos) {
		auto note = NoteOfCurrentRow();
		if(!note) return;

		static QMenu *menu = nullptr;
		delete menu;
		menu = new QMenu(this);

		auto actionMoveToGroup = menu->addAction("Переместить в группу");
		auto actionEditGroup = menu->addAction("Редактировать группу");
		auto actionGroupsSubscribes = menu->addAction("Подписки на группы");

		actionEditGroup->setEnabled(true);
		if(note->group == Note::defaultGroupName()) actionEditGroup->setEnabled(false);

		connect(actionMoveToGroup, &QAction::triggered, [note](){ note->DialogMoveToGroup(); });
		connect(actionEditGroup, &QAction::triggered, [note](){ note->DialogEditCurrentGroup(); });
		connect(actionGroupsSubscribes, &QAction::triggered, [this](){ DialogGroupsSubscribes(); });

		menu->exec(table->viewport()->mapToGlobal(pos));
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

	connect(icon, &QSystemTrayIcon::activated, [icon, this](QSystemTrayIcon::ActivationReason reason){
		if(reason == QSystemTrayIcon::Trigger) ShowMainWindow();
		if(reason == QSystemTrayIcon::Context) icon->contextMenu()->exec();
	});

	menu->addAction("Show main window");
	MyQWidget::SetFontBold(menu->actions().back(), true);
	connect(menu->actions().back(), &QAction::triggered, this, [this](){ ShowMainWindow(); });

	menu->addAction("Hide main window");
	connect(menu->actions().back(), &QAction::triggered, this, &QWidget::hide);

	menu->addSeparator();

	menu->addAction("Create new note");
	connect(menu->actions().back(), &QAction::triggered, this, [this](){ CreateNewNote(); });

	menu->addSeparator();

	menu->addAction("Close app");
	MyQWidget::SetFontBold(menu->actions().back(), true);
	connect(menu->actions().back(), &QAction::triggered, this, &QWidget::close);

	auto screens = QGuiApplication::screens();
	if(screens.size() < 2) return;

	auto screen2 = screens[1];
	QPoint posOnSct2{1871,1001};
	QPoint globalPosForIcon = screen2->geometry().topLeft() + posOnSct2;

	auto addIcon = new AdditionalTrayIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowForward), globalPosForIcon, this);
	addIcon->setContextMenu(menu);
	connect(addIcon, &ClickableQWidget::clicked, this, [this](){ ShowMainWindow(); });
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

bool WidgetMain::DialogGroupsSubscribes()
{
	auto groups = DataBase::GroupsAllFields();
	std::vector<MyQDialogs::CheckBoxDialogItem> items;
	for(auto &gr:groups)
	{
		auto &item = items.emplace_back();

		item.text = gr[Fields::nameGroupIndex];
		item.enabled = (gr[Fields::idGroupIndexInGroups] != DataBase::DefaultGroupId2()); // отключение chBox-а дефолтной группы
		item.checkState = Fields::CheckLogicField(gr[Fields::subscribedIndex]);
	}
	auto res = MyQDialogs::CheckBoxDialog("Groups subscribes", items);
	if(!res.accepted) return false;

	if(res.allItems.size() != groups.size() or items.size() != groups.size())
	{ QMbError("unexpacted error: res.allItems.size() != groups.size()"); return false; }

	groupsSubscribesValue.clear();
	for(uint i=0; i<res.allItems.size(); i++)
	{
		if(res.allItems[i].checkState != items[i].checkState)
		{
			DataBase::SetGroupSubscribed(groups[i][Fields::idGroupIndexInGroups], res.allItems[i].checkState);
		}

		if(res.allItems[i].checkState) groupsSubscribesValue += groups[i][Fields::idGroupIndexInGroups] + ":1";
		else groupsSubscribesValue += groups[i][Fields::idGroupIndexInGroups] + ":0";
	}
	if(!MyQFileDir::WriteFile(groupsSubscribesFile, groupsSubscribesValue.join(",")))
		QMbError("Error writing file " + groupsSubscribesFile);

	LoadNotes();

	return true;
}

void WidgetMain::SlotTest()
{
	//auto clone = notes[0]->note->Clone();
	//clone.SetName("123 new name " + QDateTime::currentDateTime().toString());
	//clone.SetDtLastUpdated(clone.DtLastUpdated().addSecs(+10));
	//netClient->UpdateNoteFromGetedNote(clone.ToStr_v1(), nullptr);
}

void WidgetMain::SlotMenu(QPushButton *btn)
{
	std::vector<MyQDialogs::MenuItem> items;
	items.emplace_back("Net client", [this](){ netClient->widget->showNormal(); netClient->widget->activateWindow(); });
	items.emplace_back(MyQDialogs::SeparatorMenuItem());
	items.emplace_back("Resave not saved notes", [](){  });
	items.emplace_back(MyQDialogs::SeparatorMenuItem());
	items.emplace_back("Open DB", [](){ MyQExecute::Execute(DataBase::baseDataCurrent->baseFilePathName); });
	items.emplace_back("Show DB in explorer", [](){ MyQExecute::ShowInExplorer(DataBase::baseDataCurrent->baseFilePathName); });
	items.emplace_back(MyQDialogs::SeparatorMenuItem());
	items.emplace_back("Settings.ini", [this](){ MyQExecute::Execute(settingsFile); });
	items.emplace_back("Show Settings.ini in explorer", [this](){ MyQExecute::ShowInExplorer(settingsFile); });
	items.emplace_back(MyQDialogs::SeparatorMenuItem());
	items.emplace_back("GitExtesions: open repo with DB", [this](){
		GitExtensionsTool::ExecuteGitExtensions(DataBase::baseDataCurrent->pathDataBase, true, filesPath);
	});

	MyQDialogs::MenuUnderWidget(btn, std::move(items));
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

void WidgetMain::CreateNewNote()
{
	QString newName = MyQDialogs::InputLine("Создание заметки", "Введите название новой заметки", "").text;
	if(newName.isEmpty()) return;

	auto dt = QDateTime::currentDateTime();

	Note tmpNote(newName, false, dt, dt.addSecs(3600), Note::StartText());

	auto &newNote = MakeNewNote(tmpNote, created);

	table->setCurrentCell(RowOfNote(&newNote), 0);

	WidgetNoteEditor::MakeOrShowNoteEditor(newNote);
}

void WidgetMain::ShowMainWindow()
{
	this->showNormal();
	PlatformDependent::SetTopMostFlash(this);
}

void WidgetMain::MostOpenedNotes()
{
	auto ids = DataBase::NotesIdsOrderedByOpensCount();
	std::vector<Note*> notes;
	for(auto &id:ids)
	{
		auto noteInMain = NoteById(id.toLongLong());
		if(noteInMain) notes.push_back(noteInMain->note.get());
		if(notes.size() == 20) break;
	}

	QStringList names;
	for(auto &note:notes)
	{
		names += note->Name();
	}

	auto answ = MyQDialogs::ListDialog("Most opened notes", names);
	if(answ.accepted)
	{
		Note &noteRef = *notes[answ.index];
		WidgetNoteEditor::MakeOrShowNoteEditor(noteRef);
	}
}

Note *WidgetMain::FindOriginalNote(qint64 idNoteOnServer)
{
	for(auto &note:notes)
		if(note->note->idOnServer == idNoteOnServer) return note->note.get();
	return nullptr;
}

void WidgetMain::SaveSettings()
{
	/// не надо тут пытаться сохранять задачи
	/// ибо при завершении работы программы инициированном ОС они не будут успевать сохраняться
	/// сохранение задач должно надежно происходить при их изменении

	static QByteArray prevGeo;
	static QByteArray prevHeaderState;
	auto newGeo = this->saveGeometry();
	auto newHeaderState = this->table->horizontalHeader()->saveState();
	if(prevGeo != newGeo or prevHeaderState != newHeaderState)
	{
		MyQFileDir::WriteFile(settingsFile, "");
		QSettings settings(settingsFile, QSettings::IniFormat);

		settings.setValue("geoMainWidget", newGeo);
		settings.setValue("tableHeaderState", newHeaderState);

		prevGeo = newGeo;
		prevHeaderState = newHeaderState;
	}
}

void WidgetMain::LoadSettings()
{
	if(!QFile::exists(settingsFile)) return;

	QFileInfo settingsFileInfo(settingsFile);
	QString backubPath = settingsFileInfo.path() + "/settings backups";
	QString backubFile = backubPath + "/" + QDateTime::currentDateTime().toString("yyyy.MM.dd hh_mm_ss")
												+ " " + settingsFileInfo.fileName();
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

void WidgetMain::LoadGroupsSubscribes()
{
	if(!QFile::exists(groupsSubscribesFile)) return;
	auto readRes = MyQFileDir::ReadFile2(groupsSubscribesFile);
	if(!readRes.success) QMbError("Error reading file " + groupsSubscribesFile);
	else
	{
		groupsSubscribesValue = readRes.content.split(",");
		for(auto &valPair:groupsSubscribesValue)
		{
			auto list = valPair.split(":");
			if(list.size() != 2) { QMbError("bad content in file " + groupsSubscribesFile); break; }

			QString &id = list[0];
			bool val = list[1].toUInt();

			DataBase::SetGroupSubscribed(id, val);
		}
	}
}

void WidgetMain::LoadNotes()
{
	ClearNotesInWidgetMain();
	auto notes = Note::LoadNotes();
	for(auto &note:notes)
	{
		MakeNewNote(note, loaded);
	}
}

int WidgetMain::RowOfNote(Note * note)
{
	for(uint i=0; i<notes.size(); i++)
	{
		if(note == notes[i]->note.get())
		{
			int row = table->row(notes[i]->rowView.itemName);
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
		if(notes[i]->rowView.itemName == item) return notes[i]->note.get();
	}
	QMbError("NoteOfRow: NOTE NOT FOUND for row("+QSn(row)+") and existing item " + item->text());
	return nullptr;
}

Note * WidgetMain::NoteOfCurrentRow()
{
	return NoteOfRow(table->currentRow());
}

NoteInMain * WidgetMain::NoteById(qint64 id)
{
	for(auto &note:notes)
	{
		if(note->note->id == id) return note.get();
	}
	return nullptr;
}

NoteInMain *WidgetMain::NoteByIdOnServer(qint64 idOnServer)
{
	qdbg << "NoteByIdOnServer!!! нельзя использщовать пока не откажусь от id on clinet!!!";
	for(auto &note:notes)
	{
		if(note->note->idOnServer == idOnServer) return note.get();
	}
	return nullptr;
}

int WidgetMain::NoteIndexInWidgetMainNotes(Note * note, bool showError)
{
	for(uint index=0; index<notes.size(); index++)
	{
		if(notes[index]->note.get() == note) { return index; }
	}
	if(showError) QMbError("note "+note->Name()+" not fount by NoteIndexInWidgetMainNotes");
	return -1;
}



Note & WidgetMain::MakeNewNote(Note noteSrc, newNoteReason reason)
{
	notes.emplace_back(std::unique_ptr<NoteInMain>(new NoteInMain));
	NoteInMain &newNoteInMainRef = *notes.back().get();
	newNoteInMainRef.note = std::make_unique<Note>();
	Note* newNote = newNoteInMainRef.note.get();

	newNote->InitFromTmpNote(noteSrc);

	MakeWidgetsForMainTable(newNoteInMainRef);

	if(reason == created)
	{
		newNote->id = Note::idMarkerCreateNewNote;
		newNote->SaveNoteOnClient("MakeNewNote-doSave");
	}

	auto saveNoteFoo = [newNote](void*){ newNote->SaveNoteOnClient("saveNoteFoo"); };

	newNote->AddCBNameUpdated(saveNoteFoo, &newNoteInMainRef, newNoteInMainRef.cbCounter);
	newNote->AddCBContentUpdated(saveNoteFoo, &newNoteInMainRef, newNoteInMainRef.cbCounter);
	newNote->AddCBDTUpdated(saveNoteFoo, &newNoteInMainRef, newNoteInMainRef.cbCounter);

	newNote->removeNoteWorker = [this, newNote](){
		RemoveNote(newNote, true);
		CheckNotesForAlarm();
	};

	return *newNote;
}

int WidgetMain::MakeWidgetsForMainTable(NoteInMain &newNote)
{
	NoteInMain *newNotePtr = &newNote;
	auto updateRowFoo = [this, newNotePtr](void*){ UpdateWidgetsFromNote(*newNotePtr); };

	newNote.note->AddCBNameUpdated(updateRowFoo, &newNote, newNote.cbCounter);
	newNote.note->AddCBDTUpdated(updateRowFoo, &newNote, newNote.cbCounter);
	newNote.note->AddCBGroupChanged(updateRowFoo, &newNote, newNote.cbCounter);

	auto itemName = new QTableWidgetItem;
	auto itemGroup = new QTableWidgetItem;

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

	newNote.rowView = RowView(itemName, itemGroup, chActive, dtEditNotify, dtEditPostpone);

	UpdateWidgetsFromNote(newNote);

	connect(dtEditNotify, &QDateTimeEdit::dateTimeChanged, [&newNote, dtEditPostpone](const QDateTime &datetime){
		newNote.note->SetDT(datetime, datetime);

		dtEditPostpone->blockSignals(true);
		dtEditPostpone->setDateTime(datetime);
		dtEditPostpone->blockSignals(false);
	});
	connect(dtEditPostpone, &QDateTimeEdit::dateTimeChanged, [&newNote](const QDateTime &datetime){
		newNote.note->SetDT(newNote.note->DTNotify(), datetime);
	});

	table->setRowCount(table->rowCount()+1);
	int rowIndex = table->rowCount()-1;
	table->setItem(			rowIndex, ColIndexes::name, itemName);
	table->setItem(			rowIndex, ColIndexes::group, itemGroup);
	table->setCellWidget(	rowIndex, ColIndexes::chBox, widgetForChBox);
	table->setCellWidget(	rowIndex, ColIndexes::notifyDTedit, dtEditNotify);
	table->setCellWidget(	rowIndex, ColIndexes::postponeDTedit, dtEditPostpone);
	return rowIndex;
}

void WidgetMain::UpdateWidgetsFromNote(NoteInMain &note)
{
	note.rowView.itemName->setText("   " + note.note->Name());
	note.rowView.itemGroup->setText(note.note->group);
	note.rowView.chBox->setChecked(note.note->activeNotify);
	note.rowView.dteNotify->setDateTime(note.note->DTNotify());
	note.rowView.dtePostpone->setDateTime(note.note->DTPostpone());
}

void WidgetMain::FilterNotes(const QString &textFilter)
{
	QString textFilterTranslited = MyQString::Translited(textFilter);
	QString contentHead;
	for(int row=0; row<table->rowCount(); row++)
	{
		QStringRef contentHead(&NoteOfRow(row)->Content(), 0,
							   NoteOfRow(row)->Content().size() > 2000 ? 2000 : NoteOfRow(row)->Content().size());
		if(textFilter.isEmpty()
				|| table->item(row, ColIndexes::name)->text().contains(textFilter, Qt::CaseInsensitive)
				|| table->item(row, ColIndexes::name)->text().contains(textFilterTranslited, Qt::CaseInsensitive)
				|| contentHead.contains(textFilter, Qt::CaseInsensitive)
				|| contentHead.contains(textFilterTranslited, Qt::CaseInsensitive))
		{
			table->setRowHidden(row, false);
		}
		else
		{
			table->setRowHidden(row, true);
		}
	}
}

void WidgetMain::RemoveNote(Note* note, bool execSqlRemove)
{
	auto index = NoteIndexInWidgetMainNotes(note, true);
	if(index==-1) { return; }

	if(execSqlRemove)
		if(!RemoveNoteSQLOnClient(notes[index]->note.get())) return;

	RemoveNoteInMainWidget(notes[index]->note.get());
}

bool WidgetMain::RemoveNoteSQLOnClient(Note * note)
{
	if(0) CodeMarkers::to_do("нужно добавить возможность удалять заметки на клиенте без связи с сервером, "
							 "тогда потребуется как-то хранить удалённые, "
							 "чтобы при связи с сервером передать ему сообщить о том что они были удалены");

	// если локальная заметка
	if(DataBase::IsGroupLocalByName(note->group)) {
		DataBase::RemoveNoteOnClient(QSn(note->id), true);
		return true;
	}
	// если сетевая
	else
	{
		auto answFoo = [this, note](QString &&answContent){
			if(answContent == NetConstants::success())
			{
				if(DataBase::RemoveNoteOnClient(QSn(note->id), true))
					RemoveNoteInMainWidget(note);
				else
				{
					QMbError("DataBase::RemoveNoteOnClient returned false; tryed to remove " + note->Name());
				}
			}
			else QMbError("Can't remove note, bad server answ");
		};

		netClient->RequestToServerWithWait(NetConstants::request_remove_note(), QSn(note->idOnServer), std::move(answFoo));
		return false;
	}
}

void WidgetMain::RemoveNoteInMainWidget(Note * note)
{
	if(int row = RowOfNote(note); row != -1)
	{
		table->removeRow(row);
	}

	auto index = NoteIndexInWidgetMainNotes(note, true);
	if(index==-1) { return; }
	notes.erase(notes.begin() + index);
}

void WidgetMain::ClearNotesInWidgetMain()
{
	while(table->rowCount()) table->removeRow(table->rowCount()-1);
	notes.clear();
	if(widgetAlarms) widgetAlarms->GiveNotes({});
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

void WidgetMain::SlotForNetClientNoteRemoved(qint64 id)
{
	auto note = NoteById(id);
	if(!note) QMbError("SlotForNetClientNoteRemoved note not found");
	else RemoveNote(note->note.get(), false);
}

void WidgetMain::SlotForNetClientNoteChangedGroupOrUpdated(qint64 id)
{
	auto note = NoteById(id);
	if(!note) { QMbError("SlotForNetClientNoteRemoved note not found"); return; }

	note->note->UpdateThisNoteFromSQL();
	UpdateWidgetsFromNote(*note);
}

void WidgetMain::SlotForNetClientNewNoteAppeared(qint64 id)
{
	auto rec = DataBase::NoteByIdOnClient(QSn(id));
	if(rec.isEmpty()) { QMbError("SlotForNetClientNewNoteAppeared note not found"); return; }
	else MakeNewNote(Note::CreateFromRecord(rec), loaded);
}








