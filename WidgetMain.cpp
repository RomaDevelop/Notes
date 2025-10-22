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
#include <QProgressDialog>

#include "MyCppDifferent.h"
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

#include "git.h"

#include "NetConstants.h"
#include "DataBase.h"
#include "WidgetNoteEditor.h"
#include "Settings.h"

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

	bool continueWork = GitWorkAtStart(base);
	if(!continueWork)
	{
		CloseApp();
		return;
	}

	MyQSqlDatabase::Init(base, {},
							[](const QString &str){ qdbg << str; },
							[](const QString &str){ QMbError(str); });
	DataBase::InitChildDataBase(DataBase::client);
	DataBase::BackupBase();

	Note::logWorker = [](const QString &str){ qdbg << str; };
	Note::errorWorker = [](const QString &str){ QMbError(str); };

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

		MyCppDifferent::any_guard guard(Settings::disableCbPropChanged, true, false);
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
// Lists notes
	auto btnMostOpenedNotes = new QToolButton();
	btnMostOpenedNotes->setIcon(QIcon(Resources::list_mo().GetPathName()));
	hlo1->addWidget(btnMostOpenedNotes);
	connect(btnMostOpenedNotes, &QPushButton::clicked, this, [this](){ NotesLists(INotesOwner::mostOpened); });
	auto btnRecentOpenedNotes = new QToolButton();
	btnRecentOpenedNotes->setIcon(QIcon(Resources::list_ro().GetPathName()));
	hlo1->addWidget(btnRecentOpenedNotes);
	connect(btnRecentOpenedNotes, &QPushButton::clicked, this, [this](){ NotesLists(INotesOwner::recentOpened); });
	auto btnNextAlarmsNotes = new QToolButton();
	btnNextAlarmsNotes->setIcon(QIcon(Resources::list_na().GetPathName()));
	hlo1->addWidget(btnNextAlarmsNotes);
	connect(btnNextAlarmsNotes, &QPushButton::clicked, this, [this](){ NotesLists(INotesOwner::nextAlarms); });
	auto btnNextAlarmsNotesJoin = new QToolButton();
	btnNextAlarmsNotesJoin->setIcon(QIcon(Resources::list_arrow_down().GetPathName()));
	hlo1->addWidget(btnNextAlarmsNotesJoin);
	connect(btnNextAlarmsNotesJoin, &QPushButton::clicked, this, [this](){ NotesLists(INotesOwner::nextAlarmsAlarmNow); });

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
				note->ExecRemoveNoteWorker(true);
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

	hlo1->addSpacing(10);

// Menu
	QPushButton *btnMenu = new QPushButton("Menu");
	btnMenu->setFixedWidth(QFontMetrics(btnMenu->font()).horizontalAdvance(btnMenu->text()) + 20);
	hlo1->addWidget(btnMenu);
	connect(btnMenu, &QPushButton::clicked, [this, btnMenu]() { SlotMenu(btnMenu); });

// Test
	QPushButton *btnTest = new QPushButton("Test");
	btnTest->setFixedWidth(QFontMetrics(btnTest->font()).horizontalAdvance(btnTest->text()) + 20);
	hlo1->addWidget(btnTest);
	connect(btnTest,&QPushButton::clicked, this, &WidgetMain::SlotTest);

	hlo1->addSpacing(10);

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

	hlo1->addSpacing(10);

// To tray
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

		auto actionRename = menu->addAction("Переименовать");
		menu->addSeparator();
		auto actionMoveToGroup = menu->addAction("Переместить в группу");
		auto actionEditGroup = menu->addAction("Редактировать группу");
		auto actionGroupsSubscribes = menu->addAction("Подписки на группы");

		actionEditGroup->setEnabled(true);
		if(note->groupName == Note::defaultGroupName()) actionEditGroup->setEnabled(false);

		connect(actionRename, &QAction::triggered, [note](){ note->DialogRenameNote(); });
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

	menu->addAction("Most opened notes");
	connect(menu->actions().back(), &QAction::triggered, this, [this](){ NotesLists(mostOpened); });
	menu->addAction("Recent opened notes");
	connect(menu->actions().back(), &QAction::triggered, this, [this](){ NotesLists(recentOpened); });
	menu->addAction("Next alarms notes");
	connect(menu->actions().back(), &QAction::triggered, this, [this](){ NotesLists(nextAlarms); });
	menu->addAction("Next alarms join");
	connect(menu->actions().back(), &QAction::triggered, this, [this](){ NotesLists(INotesOwner::nextAlarmsAlarmNow); });

	menu->addSeparator();

	menu->addAction("Create new note");
	connect(menu->actions().back(), &QAction::triggered, this, [this](){ CreateNewNote(); });

	menu->addSeparator();

	menu->addAction("Close app");
	MyQWidget::SetFontBold(menu->actions().back(), true);
	connect(menu->actions().back(), &QAction::triggered, this, [this](){ TrayIconSlotClose(); });

	auto screens = QGuiApplication::screens();
	if(screens.size() < 2) return;

	auto screen2 = screens[1];
	QPoint posOnSct2{1871,1001};
	QPoint globalPosForIcon = screen2->geometry().topLeft() + posOnSct2;

	auto addIcon = new AdditionalTrayIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowForward), globalPosForIcon, this);
	addIcon->setContextMenu(menu);
	connect(addIcon, &ClickableQWidget::clicked, this, [this](){ ShowMainWindow(); });
}

void WidgetMain::TrayIconSlotClose()
{
	auto answ = QMessageBox::question({}, "Завершение работы приложения", "Сделать комит перед завершением работы?");
	if(answ == QMessageBox::Yes)
	{
		GitWorkCommitAndClose();
		if(abortClose) {
			abortClose = false;
			return;
		}

	}
	closeNoQuestions = true;
	close();
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

	int secsToNextAlarm = 60*60; // в список следующих уведомлений попадают заметки до которых не более часа

	msetNotesOrderedByDTCreated alarmedNotes;
	std::map<int, vectorNotePtr> nextAlarmsNotes;

	for(auto &note:notes)
	{
		auto secsToAlarmCurrent = note->note->SecsToAlarm(currentDateTime);
		if(secsToAlarmCurrent <= 0)
		{
			alarmedNotes.insert(note->note.get());
		}
		else if(secsToAlarmCurrent <= secsToNextAlarm)
		{
			nextAlarmsNotes[secsToAlarmCurrent].emplace_back(note->note.get());
		}
	}

	widgetAlarms->AlarmNotes(std::move(alarmedNotes), std::move(nextAlarmsNotes));
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
//	auto ids = DataBase::DoSqlQueryGetFirstField("select "+Fields::idNote()+" from "+Fields::Notes()+" order by "+Fields::idNote());
//	QDateTime dt = QDateTime::currentDateTime().addDays(-275);
//	for(auto &id:ids)
//	{

//		auto r = DataBase::MakeUpdateRequestOneField(Fields::Notes(), Fields::dtCreated(), dt.toString(Fields::dtFormat()),
//													 Fields::idNote(), id);
//		DataBase::DoSqlQuery(r.first, r.second);

//		dt = dt.addDays(1);
//	}

//	auto ids = DataBase::DoSqlQueryGetFirstField("select "+Fields::idNote()+" from "+Fields::Notes()+" order by "+Fields::dtCreated());
//	int newid = -1;
//	for(auto &id:ids)
//	{
//		DataBase::DoSqlQuery("UPDATE "+Fields::Notes()+" SET "+Fields::idNote()+" = "+QSn(newid--)
//							 +" WHERE "+Fields::idNote()+" = " + id);
//	}
}

void WidgetMain::SlotMenu(QPushButton *btn)
{
	std::vector<MyQDialogs::MenuItem> items;
	items.emplace_back("Settings", [](){ DialogSettings::Execute(); });
	items.emplace_back(MyQDialogs::SeparatorMenuItem());
	items.emplace_back("Net client", [this](){ netClient->widget->showNormal(); netClient->widget->activateWindow(); });
	items.emplace_back("Synch", [this](){ netClient->request_all_notes_sending(); });
	items.emplace_back(MyQDialogs::SeparatorMenuItem());
	items.emplace_back("Resave not saved notes", [](){ QMbInfo("mock"); });
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
	items.emplace_back("Close DB, commit, push, close app", [this](){ GitWorkCommitAndClose(); });

	MyQDialogs::MenuUnderWidget(btn, std::move(items));
}

void WidgetMain::closeEvent(QCloseEvent * event)
{
	if(abortClose) {
		abortClose = false;
		event->ignore();
		return;
	}

	if(!closeNoQuestions)
	{
		auto answ = MyQDialogs::CustomDialog("Завершение работы приложения","Вы уверены, что хотите завершить работу приложения?"
																			"\n\n(уведомления на задачи не будут поступать)"
																			"\n(можно свернуть в трей, приложение продолжит работать)",
											 {"Завершить", "Сделать коммит и завершить", "Свернуть в трей", "Ничего не делать"});
		if(0){}
		else if(answ == "Завершить") {/*ничего не делаем*/}
		else if(answ == "Сделать коммит и завершить") { GitWorkCommitAndClose(); }
		else if(answ == "Свернуть в трей") { hide(); abortClose = true; }
		else if(answ == "Ничего не делать") { abortClose = true; }
		else { QMbc(0,"error", "not realesed button " + answ); abortClose = true; }
	}

	/// не надо тут пытаться сохранять задачи
	/// ибо при завершении работы программы инициированном ОС они не будут успевать сохраняться
	/// сохранение задач должно надежно происходить при их изменении

	if(abortClose) {
		abortClose = false;
		event->ignore();
		return;
	}

	SaveSettings();
	event->accept();
	QApplication::exit();
}

void WidgetMain::CloseApp()
{
	this->deleteLater();
	QTimer::singleShot(0,[](){ QApplication::exit(); });
	if(0) CodeMarkers::to_do_afterwards("в этом сценарии вылетает крит");
}

bool WidgetMain::GitWorkAtStart(BaseData &base)
{
#ifdef QT_DEBUG
	return true;
#endif

	auto answ = MyQDialogs::CustomDialog("Launching Notes", "Do you want to update repo before launching Notes?",
										 {"Yes, update", "No update", "Abort launch"});
	if(answ == "No update")
	{
		return true;
	}
	else if(answ == "Abort launch")
	{
		return false;
	}
	else if(answ == "Yes, update")
	{
		// здесь ничего, код просто идёт дальше, там будет обновление
	}
	else
	{
		QMbError("Unexpected answ");
		return true;
	}

	QProgressDialog progress("", "", 0, 3);
	progress.setWindowTitle("Работа с Git");
	progress.setLabelText("Выполнение fetch...");
	progress.setCancelButton({});
	progress.show();
	QApplication::processEvents();

	QString fetchRes;
	while(fetchRes != "finish")
	{
		for(int i=0; i<3; i++)
		{
			QProcess process;
			process.setWorkingDirectory(base.pathDataBase);
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

	progress.setLabelText("Выполнение pull");
	progress.setValue(1);
	QApplication::processEvents();

	QString pullRes;
	while(pullRes != "finish")
	{
		for(int i=0; i<3; i++)
		{
			QProcess process;
			process.setWorkingDirectory(base.pathDataBase);
			process.start("git", QStringList() << "pull");
			if(!process.waitForStarted(3000))
			{
				pullRes = "error waitForStarted " + (QStringList() << "pull").join(" ");
			}
			else if(!process.waitForFinished(3000))
			{
				pullRes = "error waitForFinished " + (QStringList() << "pull").join(" ");
			}
			else pullRes = process.readAllStandardError();

			if(pullRes.isEmpty()) break;
		}

		if(!pullRes.isEmpty())
		{
			auto answ = QMessageBox::question(nullptr, "Pull errors", "Pull did with errors:\n\n"+pullRes+"\n\nTry again?");
			if(answ == QMessageBox::No) pullRes = "finish";
		}
		else pullRes = "finish";
	}

	progress.setLabelText("Получение статуса...");
	progress.setValue(2);
	QApplication::processEvents();
	QString pathRepo = base.pathDataBase;
	auto statusAfterWork = Git::GetGitStatusForOneDir(pathRepo);

	progress.close();
	QApplication::processEvents();

	if(CheckGitStatus(statusAfterWork))
	{
		auto answ = MyQDialogs::CustomDialogWithTimer("Git extensions",
									"Fetch and pull completed successfully.\n\nPress Launch to launch Notes",
									{"Launch", "Abort launch"}, 0, 5);
		if(answ == "Launch") {}
		else if(answ == "Abort launch") {
			return false;
		}
		else QMbError("unexpected answ "+answ);
	}
	else
	{
		auto answ = QMessageBox::question({}, "Git extensions", "Fetch and push completed.\n\nWARNING:\n\nUnexpected status:"
					"\n\nCommit status: "+statusAfterWork.commitStatus+"\nPush status: "+statusAfterWork.pushStatus
										  +"\n\nLaunch Git extensions?");

		if(answ == QMessageBox::Yes)
		{
			if(GitExtensionsTool::ExecuteGitExtensions(base.pathDataBase, true, filesPath))
				QMbInfo("Launching GitExtensions...\n\nPress ok when you finish repo updating.");
			//else QMbError("Error launching GitExtensions"); // ExecuteGitExtensions выводит ошибку сам
		}
	}

	return true;
}

void WidgetMain::GitWorkCommitAndClose()
{
	DataBase::currentQSqlDb->close();

	auto CycleWithQuestion = [this](QString errorText, std::function<GitStatus()> action){
		bool cycle = true;
		while(cycle)
		{
			GitStatus gitRes = action();
			if(gitRes.success and gitRes.error.isEmpty() and gitRes.errorOutput.isEmpty())
				break;
			auto answ = MyQDialogs::CustomDialog("Error", errorText+"\nGitStatus:\n"+gitRes.ToStr(),
												 {"Try again", "Continue app work", "Close and open git extensions", "Close"});
			if(answ == "Try again") ;
			else if(answ == "Continue app work") cycle = false;
			else if(answ == "Close and open git extensions")
			{
				GitExtensionsTool::ExecuteGitExtensions(DataBase::baseDataCurrent->pathDataBase, true, filesPath);
				closeNoQuestions = true;
				cycle = false;
				close();
			}
			else if(answ == "Close")
			{
				closeNoQuestions = true;
				cycle = false;
				close();
			}
			else { QMbError("bad answ "+answ); return; }
		}
	};

	QString pathRepo = DataBase::baseDataCurrent->pathDataBase;

	QProgressDialog progress("", "", 0, 4);
	progress.setWindowTitle("Работа с Git");
	progress.setCancelButton({});
	progress.show();

	progress.setLabelText("Выполнение add...");
	progress.setValue(0);
	QApplication::processEvents();
	CycleWithQuestion("Error git add .", [pathRepo](){ return Git::DoGitCommand2(pathRepo, { "add", "." }); });

	progress.setLabelText("Выполнение commit...");
	progress.setValue(1);
	QApplication::processEvents();
	CycleWithQuestion("Error git commit -m Update", [pathRepo](){ return Git::DoGitCommand2(pathRepo, { "commit", "-m", "Update" }); });

	progress.setLabelText("Выполнение push...");
	progress.setValue(2);
	QApplication::processEvents();
	CycleWithQuestion("Error git push github master", [pathRepo](){
		auto gitRes = Git::DoGitCommand2(pathRepo, { "push", "github", "master" });
		if(gitRes.success)
		{
			while (gitRes.errorOutput.endsWith('\\') or gitRes.errorOutput.endsWith('\n') or gitRes.errorOutput.endsWith('n')) {
				gitRes.errorOutput.chop(1);
			}
			if( (gitRes.errorOutput.startsWith("To https://github.com") and gitRes.errorOutput.endsWith("master -> master"))
					or gitRes.errorOutput == "Everything up-to-date")
			{
				gitRes.errorOutput.clear();
			}
		}
		return gitRes;
	});

	progress.setLabelText("Получение статуса...");
	progress.setValue(3);
	QApplication::processEvents();
	auto statusAfterWork = Git::GetGitStatusForOneDir(pathRepo);

	progress.close();
	QApplication::processEvents();

	closeNoQuestions = false;
	QString question = "Add, commit and push completed successfully.\n\nClose app?";
	int defButtonIndex = 0;
	if(CheckGitStatus(statusAfterWork) == false)
	{
		question = "Work (add, commit, push) completed with.\n\nWARNING:\n\nUnexpected status:"
				   "\nCommit status: "+statusAfterWork.commitStatus+"\nPush status: "+statusAfterWork.pushStatus
				   +"\n\nClose app?";
		defButtonIndex = -1;
	}

	auto answ = MyQDialogs::CustomDialogWithTimer("Finishing work", question, {"Yes", "No"}, defButtonIndex, 5);
	if(answ == "Yes")
	{
		closeNoQuestions = true;
		close();
	}
	else abortClose = true;
}

bool WidgetMain::CheckGitStatus(const GitStatus &status)
{
	return status.commitStatus == Statuses::commited() and status.pushStatus == Statuses::pushed();
}

void WidgetMain::CreateNewNote()
{
	QString newName = MyQDialogs::InputLine("Создание заметки", "Введите название новой заметки", "").text;
	if(newName.isEmpty()) return;

	auto dt = QDateTime::currentDateTime();

	Note tmpNote(newName, false, dt, dt.addSecs(3600), Note::StartText());
	tmpNote.SetDTCreated(QDateTime::currentDateTime());

	auto insRes = DataBase::InsertNoteInDB(&tmpNote, true);
	if(!insRes.isEmpty()) { QMbError("CreateNewNote error insert result: "+insRes); return; }

	auto &newNote = MakeNewNote(tmpNote);

	table->setCurrentCell(RowOfNote(&newNote), 0);

	WidgetNoteEditor::MakeOrShowNoteEditor(newNote);
}

void WidgetMain::ShowMainWindow()
{
	this->showNormal();
	PlatformDependent::SetTopMostFlash(this);
}

void WidgetMain::NotesLists(lists list)
{
	QStringList ids;
	QString caption;
	if(list == mostOpened)
	{
		ids = DataBase::NotesIdsOrderedByOpensCount();
		caption = "Most opened notes";
	}
	else if (list == recentOpened) {
		ids = DataBase::NotesIdsOrderedByLastOpened();
		caption = "Last opened notes";
	}
	else if (list == nextAlarms or list == nextAlarmsAlarmNow) {
		ids = DataBase::NotesIdsOrderedByDtPostpone();
		caption = "Next alarms notes";
	}
	else { QMbError("Unrealised action " + MyQString::AsDebug(list)); }

	std::vector<Note*> notesToShow;
	for(auto &id:ids)
	{
		auto noteInMain = NoteById(id.toLongLong());
		if(noteInMain)
		{
			if(list == nextAlarms or list == nextAlarmsAlarmNow)
				if(noteInMain->note->DTPostpone() < QDateTime::currentDateTime())
					continue;

			notesToShow.push_back(noteInMain->note.get());
		}
		if(notesToShow.size() == 20) break;
	}

	QStringList rows;
	for(auto &note:notesToShow)
	{
		rows += note->Name_DTNotify_DTPospone();
	}

	enum showWay { listDialogToOpenEditor, chBoxDialogToAlarmNow };
	showWay showWayVal = listDialogToOpenEditor;
	if(list == nextAlarmsAlarmNow) showWayVal = chBoxDialogToAlarmNow;

	if(showWayVal == listDialogToOpenEditor)
	{
		auto answ = MyQDialogs::ListDialog(caption, rows, "Open note", "Close list", 920);
		if(answ.accepted)
		{
			Note &noteRef = *notesToShow[answ.index];
			WidgetNoteEditor::MakeOrShowNoteEditor(noteRef);
		}
	}
	else if(showWayVal == chBoxDialogToAlarmNow)
	{
		auto answ = MyQDialogs::CheckBoxDialog("Choose alarms to alarm now", rows, {}, {}, false, 920);
		if(answ.accepted)
		{
			std::vector<Note*> notesToJoinAlarms;
			for(auto &index:answ.checkedIndexes)
			{
				notesToJoinAlarms.push_back(notesToShow[index]);
			}

			for(auto &note:notesToJoinAlarms)
			{
				note->SetDTPostpone(QDateTime::currentDateTime());
			}
		}
	}
	else { QMbError("Unrealised showWay " + MyQString::AsDebug(showWayVal)); }
}

std::vector<Note *> WidgetMain::Notes(std::function<bool (Note *)> filter)
{
	std::vector<Note *> ret;
	if(filter) {
		for(auto &note:notes)
			if(filter(note->note.get()))
				ret.push_back(note->note.get());
	}
	else {
		for(auto &note:notes)
			ret.push_back(note->note.get());
	}
	return ret;
}

bool WidgetMain::IsNoteValid(Note *note)
{
	for(auto &validNote:notes)
		if(validNote->note.get() == note) return true;
	if(0) CodeMarkers::can_be_optimized("should make set and find in it");
	return false;
}

Note *WidgetMain::FindOriginalNote(qint64 idNote)
{
	for(auto &note:notes)
		if(note->note->id == idNote) return note->note.get();
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
	if(prevGeo != newGeo
			or prevHeaderState != newHeaderState
			or Settings::haveNotSavedChanges)
	{
		MyQFileDir::WriteFile(settingsFile, "");
		QSettings settings(settingsFile, QSettings::IniFormat);

		settings.setValue("geoMainWidget", newGeo);
		settings.setValue("tableHeaderState", newHeaderState);
		prevGeo = newGeo;
		prevHeaderState = newHeaderState;

		settings.setValue("settings", Settings::SaveToString());
		Settings::haveNotSavedChanges = false;
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

	if(settings.contains("settings"))
		Settings::LoadFromString(settings.value("settings").toString());
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
		MakeNewNote(note);
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

	if(0) CodeMarkers::to_do("make map to fast find NoteOfRow");

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

int WidgetMain::NoteIndexInWidgetMainNotes(Note * note, bool showError)
{
	for(uint index=0; index<notes.size(); index++)
	{
		if(notes[index]->note.get() == note) { return index; }
	}
	if(showError) QMbError("note "+note->Name()+" not fount by NoteIndexInWidgetMainNotes");
	return -1;
}



Note & WidgetMain::MakeNewNote(Note noteSrc)
{
	notes.emplace_back(std::unique_ptr<NoteInMain>(new NoteInMain));
	NoteInMain &newNoteInMainRef = *notes.back().get();
	newNoteInMainRef.note = std::make_unique<Note>();
	Note* newNote = newNoteInMainRef.note.get();

	newNote->InitFromTmpNote(noteSrc);

	MakeWidgetsForMainTable(newNoteInMainRef);

	auto makeSaveMoteFoo = [newNote](QString saveMsg){
		return [newNote, saveMsg = std::move(saveMsg)](void*){ newNote->SaveNoteOnClient(saveMsg); };
	};

	newNote->AddCBNameUpdated(makeSaveMoteFoo("name updated"), &newNoteInMainRef, newNoteInMainRef.cbCounter);
	newNote->AddCBContentUpdated(makeSaveMoteFoo("content updated"), &newNoteInMainRef, newNoteInMainRef.cbCounter);
	newNote->AddCBDTUpdated(makeSaveMoteFoo("dt updated"), &newNoteInMainRef, newNoteInMainRef.cbCounter);

	newNote->removeNoteWorker = [this, newNote](bool execSqlRemove){
		RemoveNote(newNote, execSqlRemove);
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
	note.rowView.itemGroup->setText(note.note->groupName);
	note.rowView.chBox->setChecked(note.note->activeNotify);
	note.rowView.dteNotify->setDateTime(note.note->DTNotify());
	note.rowView.dtePostpone->setDateTime(note.note->DTPostpone());
}

void WidgetMain::FilterNotes(const QString &textFilter)
{
	QString textFilterTranslited = MyQString::Translited(textFilter);

	for(int row=0; row<table->rowCount(); row++)
	{
		if(NoteOfRow(row)->CheckNoteForFilters(textFilter, textFilterTranslited))
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
	if(DataBase::IsGroupLocalByName(note->groupName)) {
		DataBase::RemoveNote(QSn(note->id), true);
		return true;
	}
	// если сетевая
	else
	{
		auto answFoo = [this, note](QString &&answContent){
			if(answContent == NetConstants::success())
			{
				if(DataBase::RemoveNote(QSn(note->id), true))
					RemoveNoteInMainWidget(note);
				else
				{
					QMbError("DataBase::RemoveNoteOnClient returned false; tryed to remove " + note->Name());
				}
			}
			else QMbError("Can't remove note, bad server answ");
		};

		netClient->RequestToServerWithWait(NetConstants::request_remove_note(), QSn(note->id), std::move(answFoo));
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
	if(widgetAlarms) widgetAlarms->AlarmNotes({}, {});
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
	else note->note->ExecRemoveNoteWorker(false);
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
	auto rec = DataBase::NoteById(QSn(id));
	if(rec.isEmpty()) { QMbError("SlotForNetClientNewNoteAppeared note not found"); return; }
	else MakeNewNote(Note::CreateFromRecord(rec));
}








