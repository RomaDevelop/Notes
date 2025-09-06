#include "WidgetAlarms.h"

#include <QVBoxLayout>
#include <QCloseEvent>
#include <QSettings>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QScrollBar>
#include <QWindow>

#include "MyQShortings.h"
#include "MyQTableWidget.h"
#include "MyQFileDir.h"
#include "MyQDifferent.h"
#include "MyQDialogs.h"
#include "MyQString.h"
#include "PlatformDependent.h"
#include "declare_struct.h"

#include "FastActions.h"
#include "WidgetNoteEditor.h"
#include "Resources.h"
#include "DialogInputTime.h"

const QString& RescheduleCaption() { static QString str = " Перенести на ... "; return str; }
const QString& RostponeCaption() { static QString str = " Отложить на ... "; return str; }
const QString& RemoveCaption() { static QString str = " Удалить "; return str; }
const QString& AllButtonsCaptions() {
	static QString str = QString(RescheduleCaption()).append(RostponeCaption()).append(RemoveCaption()); return str; }

const int secondsInDay = 60*60*24;

WidgetAlarms::WidgetAlarms(INotesOwner *aNotesOwner, QFont fontForLabels, QWidget *parent):
	QWidget(parent),
	fontForLabels{fontForLabels},
	fontMetrixForLabels(fontForLabels),
	notesOwner {aNotesOwner}
{
	settingsFile = MyQDifferent::PathToExe() + "/files/settings_widget_alarms.ini";

	setWindowFlag(Qt::WindowMaximizeButtonHint, false);
	setWindowFlag(Qt::WindowCloseButtonHint, false);
	setWindowFlag(Qt::WindowDoesNotAcceptFocus, true);

	QVBoxLayout *vlo_main = new QVBoxLayout(this);
	QHBoxLayout *hlo1 = new QHBoxLayout;

	vlo_main->addLayout(hlo1);

	table = new QTableWidget;
	table->setColumnCount(1);
	table->verticalHeader()->hide();
	table->horizontalHeader()->hide();
	hlo1->addWidget(table);
	connect(table, &QTableWidget::cellDoubleClicked, [this](int r, int){
		WidgetNoteEditor::MakeOrShowNoteEditor(*notes[r]->note);
	});

	CreateBottomRow(vlo_main);
	CreateFindSection(vlo_main);

	auto timerGeoSaver = new QTimer(this);
	connect(timerGeoSaver, &QTimer::timeout, [this](){
		if(isVisible()) geo = saveGeometry();
	});
	timerGeoSaver->start(200);

	CreateTableContextMenu();

	InitFitColWidthTimer();
	InitTimerSetterLabels();
	InitMessageForNotifyTimer();
}

WidgetAlarms::~WidgetAlarms()
{
	if(!geo.isEmpty())
	{
		QDir().mkpath(QFileInfo(settingsFile).path());
		MyQFileDir::WriteFile(settingsFile, "");
		QSettings settings(settingsFile, QSettings::IniFormat);

		auto geo = saveGeometry();
		settings.setValue("geo", geo);

		QWidget newWidget;
		newWidget.restoreGeometry(geo);

//		auto geoRect = geometry();
//		QString geoStr = "X:" + QSn(geoRect.x()) + " Y: " + QSn(geoRect.y())
//				+ " Width: " + QSn(geoRect.width()) + " Height: " + QSn(geoRect.height()) + "\n";
//		QString log = QDateTime::currentDateTime().toString(DateTimeFormat) +
//				" WidgetAlarms::SaveSettings saved geo: " + geoStr + "\n";
//		qdbg << " WidgetAlarms::SaveSettings saved geo: " + geoStr + "\n";
//		MyQFileDir::AppendFile(QFileInfo(settingsFile).path() + "/save geo log.txt", log);
	}
}

void WidgetAlarms::AlarmNotes(const std::vector<Note *> & notesToAlarm, Note *nextAlarmNote)
{
	for(uint i=0; i<notes.size();)
	{
		if(std::find(notesToAlarm.begin(), notesToAlarm.end(), notes[i]->note) == notesToAlarm.end())
			RemoveNoteFromWidgetAlarms(i);
		else ++i;
	}

	bool added = false;
	for(auto &newNote:notesToAlarm)
	{
		if(NoteInAlarms *findedNote = FindNote(newNote); findedNote == nullptr)
		{
			AddNote(newNote, true);
			added = true;
		}
	}

	setWindowTitle(QSn(notes.size()) + " alarms for notes");

	if(!nextAlarmNote) labelNextAlarm->clear();
	else
	{
		static QString text;
		static QTime t(0,0,0);

		auto &name = nextAlarmNote->Name();

		text.clear();
		text.append("Next alarm in ").append(t.addSecs(nextAlarmNote->SecsToAlarm(QDateTime::currentDateTime())).toString("hh:mm:ss"));
		text.append(" - ").append(name.size() <= 20 ? name : name.left(18).append("..."));

		labelNextAlarm->setText(text);
		labelNextAlarm->setToolTip(name);
	}

	if(!notes.empty())
	{
		show();
		if(added)
		{
			PlatformDependent::SetTopMostFlash(this);
			if(isMinimized()) this->windowHandle()->alert(5000);
			else PlatformDependent::SetTopMostFlash(this);
		}
	}
	else hide();
}

void WidgetAlarms::CreateTableContextMenu()
{
	table->setContextMenuPolicy(Qt::CustomContextMenu);

	connect(table, &QWidget::customContextMenuRequested, [this](const QPoint& pos) {
		auto note = NoteOfCurrentRow(false);
		if(!note) return;

		static QMenu *menu = nullptr;
		delete menu;
		menu = new QMenu(this);

		auto actionRename = menu->addAction("Переименовать");

		connect(actionRename, &QAction::triggered, [note](){ note->DialogRenameNote(); });

		menu->exec(table->viewport()->mapToGlobal(pos));
	});
}

void WidgetAlarms::CreateBottomRow(QBoxLayout *loMain)
{
	QHBoxLayout *hlo = new QHBoxLayout;
	loMain->addLayout(hlo);

	auto btnShowMainWindow = new QToolButton();
	btnShowMainWindow->setIcon(QIcon(Resources::list().GetPathName()));
	hlo->addWidget(btnShowMainWindow);
	connect(btnShowMainWindow, &QPushButton::clicked, this, [this](){ notesOwner->ShowMainWindow(); });

	auto btnMostOpenedNotes = new QToolButton();
	btnMostOpenedNotes->setIcon(QIcon(Resources::list_mo().GetPathName()));
	hlo->addWidget(btnMostOpenedNotes);
	connect(btnMostOpenedNotes, &QPushButton::clicked, this, [this](){ notesOwner->NotesLists(INotesOwner::mostOpened); });

	auto btnRecentOpenedNotes = new QToolButton();
	btnRecentOpenedNotes->setIcon(QIcon(Resources::list_ro().GetPathName()));
	hlo->addWidget(btnRecentOpenedNotes);
	connect(btnRecentOpenedNotes, &QPushButton::clicked, this, [this](){ notesOwner->NotesLists(INotesOwner::recentOpened); });

	auto btnNextAlarmsNotes = new QToolButton();
	btnNextAlarmsNotes->setIcon(QIcon(Resources::list_na().GetPathName()));
	hlo->addWidget(btnNextAlarmsNotes);
	connect(btnNextAlarmsNotes, &QPushButton::clicked, this, [this](){ notesOwner->NotesLists(INotesOwner::nextAlarms); });

	auto btnNextAlarmsNotesJoin = new QToolButton();
	btnNextAlarmsNotesJoin->setIcon(QIcon(Resources::list_arrow_down().GetPathName()));
	hlo->addWidget(btnNextAlarmsNotesJoin);
	connect(btnNextAlarmsNotesJoin, &QPushButton::clicked, this, [this](){ notesOwner->NotesLists(INotesOwner::nextAlarmsAlarmNow); });

	auto btnFind = new QToolButton();
	btnFind->setIcon(QIcon(Resources::find().GetPathName()));
	hlo->addWidget(btnFind);
	connect(btnFind, &QPushButton::clicked, this, [this](){ SlotBtnFindClicked(); });

	auto btnAdd = new QToolButton();
	btnAdd->setIcon(QIcon(Resources::add().GetPathName()));
	hlo->addWidget(btnAdd);
	connect(btnAdd, &QPushButton::clicked, this, [this](){ notesOwner->CreateNewNote(); });

	auto btnFastActions = new QToolButton();
	btnFastActions->setIcon(QIcon(Resources::action().GetPathName()));
	hlo->addWidget(btnFastActions);
	connect(btnFastActions, &QPushButton::clicked, [this, btnFastActions](){
		if(auto note = NoteOfCurrentRow(true); note)
			note->ShowMenuFastActions(btnFastActions);
	});

	hlo->addSpacing(10);

	labelNextAlarm = new QLabel;
	hlo->addWidget(labelNextAlarm);

	hlo->addStretch();

	auto btnRescheduleAll = new QPushButton(" Перенести все на ... ");
	auto btnPostponeAll = new QPushButton(" Отложить все на ... ");
	hlo->addWidget(btnRescheduleAll);
	hlo->addWidget(btnPostponeAll);
	connect(btnRescheduleAll, &QPushButton::clicked, [this, btnRescheduleAll](){
		auto action = NoteForPostponeAll();
		if(btnRescheduleAll->text().contains("выбранные")) action = NoteForPostponeSelected();
		ShowMenuPostpone(btnRescheduleAll->mapToGlobal(QPoint(0, btnRescheduleAll->height())),
						 changeDtNotify, action);
	});

	connect(btnPostponeAll, &QPushButton::clicked, [this, btnPostponeAll](){
		auto action = NoteForPostponeAll();
		if(btnPostponeAll->text().contains("выбранные")) action = NoteForPostponeSelected();
		ShowMenuPostpone(btnPostponeAll->mapToGlobal(QPoint(0, btnPostponeAll->height())),
						 setPostpone, action);
	});

	connect(table, &QTableWidget::itemSelectionChanged, this, [this, btnRescheduleAll, btnPostponeAll](){
		if(GetSelectedNotes().size() > 1)
			{ btnRescheduleAll->setText(" Перенести выбранные на ... "); btnPostponeAll->setText(" Отложить выбранные на ... "); }
		else { btnRescheduleAll->setText(" Перенести все на ... "); btnPostponeAll->setText(" Отложить все на ... "); }
	});
}

void WidgetAlarms::CreateFindSection(QBoxLayout *loMain)
{
	auto widgetFind = new QWidget;
	widgetFind->setVisible(false);
	loMain->addWidget(widgetFind);

	auto lineEditFind = new QLineEdit;

	auto tableFind = new QTableWidget;
	tableFind->setEditTriggers(QTableWidget::NoEditTriggers);
	tableFind->setColumnCount(1);
	tableFind->verticalScrollBar()->hide();
	tableFind->verticalHeader()->hide();
	tableFind->horizontalHeader()->hide();
	//tableFind->setFixedHeight(100);

	// обработчик нажания на кнопку Лупа
	SlotBtnFindClicked = [this, widgetFind, lineEditFind, tableFind]() {
		this->setFixedWidth(width());
		table->setFixedHeight(table->height());

		widgetFind->setVisible(!widgetFind->isVisible());

		QTimer::singleShot(10,[this, widgetFind, lineEditFind, tableFind](){
			adjustSize();
			tableFind->setColumnWidth(0, tableFind->width());
			if(!widgetFind->isVisible()) // если сбросить и FixedWidth FixedHeight когда widgetFind отображается окно уродуется
			{
				table->setMinimumHeight(0);
				table->setMaximumHeight(QWIDGETSIZE_MAX);
				this->setMinimumWidth(0);
				this->setMaximumWidth(QWIDGETSIZE_MAX);
			}
			else
			{
				PlatformDependent::FlashClickOnTitle(this);
				lineEditFind->setFocus();
			}
		});
	};

	auto vloFind = new QVBoxLayout(widgetFind);
	vloFind->setContentsMargins(0,0,0,0);

	auto hlo1 = new QHBoxLayout;
	hlo1->setContentsMargins(0,0,0,0);
	vloFind->addLayout(hlo1);

	auto FillingTableSearchRes = [tableFind](std::vector<Note*> notes){
		tableFind->setRowCount(notes.size());
		int row = 0;
		for(auto &note:notes)
		{
			auto item = new QTableWidgetItem(note->Name_DTNotify_DTPospone().prepend("  "));
			item->setData(Qt::UserRole, (qlonglong)note);
			tableFind->setItem(row++, 0, item);
		}
	};

	hlo1->addWidget(lineEditFind);
	connect(lineEditFind, &QLineEdit::textChanged, [this, tableFind, FillingTableSearchRes](const QString &text){
		tableFind->setRowCount(0);
		if(text.isEmpty()) return;

		QString textTranslited = MyQString::Translited(text);

// выборка заметок
		auto notes = notesOwner->Notes([text, textTranslited](Note *note){
			if(note->CheckNoteForFilters(text, textTranslited))
				return true;
			else return false;
		});

		FillingTableSearchRes(notes);
	});

	auto btnClearSearch = new QToolButton;
	btnClearSearch->setIcon(QApplication::style()->standardIcon(QStyle::StandardPixmap::SP_TitleBarCloseButton));
	hlo1->addWidget(btnClearSearch);
	connect(btnClearSearch, &QToolButton::clicked, [lineEditFind](){ lineEditFind->clear(); });

	auto btnClearRefresh = new QToolButton;
	btnClearRefresh->setIcon(QApplication::style()->standardIcon(QStyle::StandardPixmap::SP_BrowserReload));
	hlo1->addWidget(btnClearRefresh);
	connect(btnClearRefresh, &QToolButton::clicked, [lineEditFind](){ emit lineEditFind->textChanged(lineEditFind->text()); });

	auto btnHideSearch = new QToolButton;
	btnHideSearch->setIcon(QApplication::style()->standardIcon(QStyle::StandardPixmap::SP_TitleBarShadeButton));
	hlo1->addWidget(btnHideSearch);
	connect(btnHideSearch, &QToolButton::clicked, [this](){ SlotBtnFindClicked(); });

	hlo1->addStretch();

	connect(tableFind, &QTableWidget::itemDoubleClicked, this, [this, tableFind](QTableWidgetItem *item){
		Note *notePtr = (Note*)item->data(Qt::UserRole).toLongLong();
		if(!notesOwner->IsNoteValid(notePtr)) {
			QMbInfo("This note is invalid now");
			tableFind->removeRow(item->row());
			return;
		}

		WidgetNoteEditor::MakeOrShowNoteEditor(*notePtr);
	});

	vloFind->addWidget(tableFind);
}

NoteInAlarms * WidgetAlarms::FindNote(Note * noteToFind)
{
	for(auto &note:notes)
		if(note->note == noteToFind) return note.get();
	return nullptr;
}

void WidgetAlarms::AddNote(Note * note, bool addInTop, bool disableFeatureMessage)
{
	auto widget = new QWidget;

	NoteInAlarms *newNoteInAlarmsPtr = nullptr;

	if(addInTop)
	{
		int row = 0;
		table->insertRow(row);
		table->setCellWidget(row, 0, widget);
		table->scrollToTop();
		notes.insert(notes.begin(), std::make_unique<NoteInAlarms>());
		newNoteInAlarmsPtr = notes.front().get();
	}
	else
	{
		int row = table->rowCount();
		table->setRowCount(row+1);
		table->setCellWidget(row, 0, widget);
		table->scrollToBottom();
		newNoteInAlarmsPtr = notes.emplace_back(std::make_unique<NoteInAlarms>()).get();
	}

	if(!newNoteInAlarmsPtr) { QMbError("invalid newNoteInAlarmsPtr"); return; }

	newNoteInAlarmsPtr->note = note;
	newNoteInAlarmsPtr->widgetAll = widget;
	newNoteInAlarmsPtr->widgetAllExeptLabels = new QWidget;

	auto hlo = new QHBoxLayout(widget);
	hlo->setContentsMargins(0,0,0,0);
	hlo->setSpacing(0);
	auto hlo2 = new QHBoxLayout(newNoteInAlarmsPtr->widgetAllExeptLabels);
	hlo2->setContentsMargins(0,0,0,0);

	newNoteInAlarmsPtr->labelName = new QLabel;
	newNoteInAlarmsPtr->labelName->setFont(fontForLabels);
	newNoteInAlarmsPtr->labelName->setMinimumWidth(30);

	newNoteInAlarmsPtr->labelDots = new QLabel("...");
	newNoteInAlarmsPtr->labelDots->setFont(fontForLabels);
	newNoteInAlarmsPtr->labelDots->hide();

	newNoteInAlarmsPtr->labelDate = new QLabel;
	newNoteInAlarmsPtr->labelDate->setFont(fontForLabels);

	auto btnReschedule = new QPushButton(RescheduleCaption());
	auto btnPostpone = new QPushButton(RostponeCaption());
	auto btnRemove = new QPushButton(RemoveCaption());

	hlo->addWidget(newNoteInAlarmsPtr->labelName);
	hlo->addWidget(newNoteInAlarmsPtr->labelDots);
	hlo->addWidget(newNoteInAlarmsPtr->labelDate);
	hlo->addStretch();

	hlo->addWidget(newNoteInAlarmsPtr->widgetAllExeptLabels);

	hlo2->addWidget(btnReschedule);
	hlo2->addWidget(btnPostpone);
	hlo2->addWidget(btnRemove);
	hlo2->addSpacing(4);

	auto cb = [this, newNoteInAlarmsPtr](void*){
		if(newNoteInAlarmsPtr->note->SecsToAlarm(QDateTime::currentDateTime()) > 0)
		{
			RemoveNoteFromWidgetAlarms(NoteIndex(newNoteInAlarmsPtr->note));
			return;
		}
		SetLabelText(*newNoteInAlarmsPtr);
	};
	note->AddCBNameUpdated(cb, newNoteInAlarmsPtr, newNoteInAlarmsPtr->cbCounter);
	note->AddCBDTUpdated(cb, newNoteInAlarmsPtr, newNoteInAlarmsPtr->cbCounter);

	connect(btnReschedule, &QPushButton::clicked, [this, btnReschedule, note](){
		bool workCurrentNote = true;
		if(GetSelectedNotes().size() > 1)
		{
			auto answ = MyQDialogs::CustomDialog("Notes working", "Selected more one note. Reschedule all selected notes or current?",
												 {"All selected", "Current"});
			if(answ == "All selected") workCurrentNote = false;
			else if(answ == "Current") workCurrentNote = true;
			else QMbError("dialog error");
		}

		if(workCurrentNote)
			ShowMenuPostpone(btnReschedule->mapToGlobal(QPoint(0, btnReschedule->height())), changeDtNotify, note);
		else ShowMenuPostpone(btnReschedule->mapToGlobal(QPoint(0, btnReschedule->height())), changeDtNotify, NoteForPostponeSelected());
	});
	connect(btnPostpone, &QPushButton::clicked, [this, btnPostpone, note](){
		bool workCurrentNote = true;
		if(GetSelectedNotes().size() > 1)
		{
			auto answ = MyQDialogs::CustomDialog("Notes working", "Selected more one note. Pospone all selected notes or current?",
												 {"All selected", "Current"});
			if(answ == "All selected") workCurrentNote = false;
			else if(answ == "Current") workCurrentNote = true;
			else QMbError("dialog error");
		}

		if(workCurrentNote)
			ShowMenuPostpone(btnPostpone->mapToGlobal(QPoint(0, btnPostpone->height())), setPostpone, note);
		else ShowMenuPostpone(btnPostpone->mapToGlobal(QPoint(0, btnPostpone->height())), setPostpone, NoteForPostponeSelected());
	});
	connect(btnRemove, &QPushButton::clicked, [this, note](){
		if(QMessageBox::question(0,"Remove note","Removing note "+note->Name()+"\n\nAre you shure?") == QMessageBox::Yes)
		{
			RemoveNoteFromWidgetAlarms(NoteIndex(note));
			note->ExecRemoveNoteWorker();
		}
	});

	if(!disableFeatureMessage)
	{
		bool featureMsgForNotify = Features::CheckFeature(note->Content(), Features::messageForNotify());
		if(featureMsgForNotify) notesToShowMessageForNotify.push_back(note);
	}

	fitColWidthRequest = true;

	notesToSetLabel.push_back(newNoteInAlarmsPtr);
	setLabelRequestDt = QDateTime::currentDateTime();
}

void WidgetAlarms::MoveNoteUp(Note& note)
{
	RemoveNoteFromWidgetAlarms(NoteIndex(&note));
	AddNote(&note, true, true);
}

void WidgetAlarms::SetLabelText(NoteInAlarms & note)
{
	QString text1 = "   " + note.note->Name();
	QString text2 = "  ("+note.note->DTNotify().toString(DateTimeFormat_rus)+")";
	note.labelName->setText(text1);
	note.labelDate->setText(text2);
	note.labelDots->hide();

	int labelNameWidth = table->width() - note.widgetAllExeptLabels->width();
	if(table->verticalScrollBar()->isVisible()) labelNameWidth -= table->verticalScrollBar()->width();
	labelNameWidth -= note.labelDate->width() + 20;

	if(labelNameWidth > note.labelName->sizeHint().width()) labelNameWidth = note.labelName->sizeHint().width();
	if(labelNameWidth < note.labelName->sizeHint().width())
	{
		note.labelDots->show();
		labelNameWidth -= note.labelDots->sizeHint().width();
	}

	if(labelNameWidth < 30) labelNameWidth = 30;
	note.labelName->setMaximumWidth(labelNameWidth);
}

int WidgetAlarms::NoteIndex(Note *note)
{
	for(uint i=0; i<notes.size(); i++)
		if(notes[i]->note == note) return i;
	return -1;
}

void WidgetAlarms::RemoveNoteFromWidgetAlarms(int index)
{
	if(index < 0 or index >= (int)notes.size())
	{
		QMbError("RemoveNoteFromWidgetAlarms: note get bad index: " + QSn(index));
		return;
	}

	if(notesOwner->IsNoteValid(notes[index]->note))
		notes[index]->note->RemoveCbs(notes[index].get(), notes[index]->cbCounter);

	int scrollBarValue = table->verticalScrollBar()->value();
	int currentRow = table->currentRow();

	table->removeRow(index);

	if(currentRow >= 0 and currentRow < table->rowCount())
	{
		// need to restore current row because of idiotic QTableView behavior.
		// Removing row 5 or other changes current row from 1 to 2 or other
		if(index < currentRow) currentRow--;
		if(currentRow >= 0)
			table->setCurrentCell(currentRow, 0);
	}
	table->verticalScrollBar()->setValue(scrollBarValue);

	notes.erase(notes.begin() + index);

	QTimer::singleShot(10,this,[this]{ FitColWidth(); });
}

QDateTime AddSecsFromToday(const QDateTime &dt, qint64 secs)
{

	QDateTime newDT(QDate::currentDate(), dt.time());	// get today with dt time
	newDT = newDT.addSecs(secs);						// add secs
	return newDT;
}

QDateTime AddSecsFromNow(qint64 secs)
{
	return QDateTime::currentDateTime().addSecs(secs);
}

namespace ForPostpone_ns {
	const int handInput = -1;
	const int separator = -2;
}

void WidgetAlarms::ShowMenuPostpone(QPoint pos, menuPostponeCase menuPostponeCaseValue, Note* note)
{
	static QMenu *menu = nullptr;
	static menuPostponeCase menuPostponeCaseCurrent;
	menuPostponeCaseCurrent = menuPostponeCaseValue;
	delete menu;
	menu = new QMenu(this);
	declare_struct_2_fields_move(Delay, QString, text, int, seconds);
	std::vector<Delay> delays;
	int daysInCurrnetMonth = QDate::currentDate().daysInMonth();

	if(menuPostponeCaseCurrent == menuPostponeCase::changeDtNotify)
		delays = 		{{"1 день", secondsInDay}, {"2 дня", secondsInDay*2}, {"3 дня", secondsInDay*3}, {"4 дня", secondsInDay*4},
						 {"5 дней", secondsInDay*5}, {"6 дней", secondsInDay*6}, {"7 дней", secondsInDay*7}, {"8 дней", secondsInDay*8},
						 {"9 дней", secondsInDay*9}, {"10 дней", secondsInDay*10}, {"11 дней", secondsInDay*11}, {"12 дней", secondsInDay*12},
						 {"13 дней", secondsInDay*13}, {"14 дней", secondsInDay*14}, {"15 дней", secondsInDay*15},
						 {"", ForPostpone_ns::separator}, {"18 дней", secondsInDay*18},
						 {"21 дней", secondsInDay*21}, {"25 дней", secondsInDay*25}, {"Месяц", secondsInDay*daysInCurrnetMonth},
						 {"", ForPostpone_ns::separator},
						 {"40 дней", secondsInDay*40}, {"50 дней", secondsInDay*50}, {"Два месяца", secondsInDay*daysInCurrnetMonth*2},
						 {"Ввести вручную", ForPostpone_ns::handInput},
						 };
	else if(menuPostponeCaseCurrent == menuPostponeCase::setPostpone)
		delays = 		{{"5 минут", 60*5}, {"10 минут", 60*10}, {"15 минут", 60*15}, {"20 минут", 60*20},
						 {"25 минут", 60*25}, {"30 минут", 60*30}, {"35 минут", 60*35}, {"40 минут", 60*40},
						 {"45 минут", 60*45}, {"50 минут", 60*50}, {"1 час", 60*60}, {"1,5 часа", 60*90},
						 {"2 часа", 60*60*2}, {"3 часа", 60*60*3}, {"4 часа", 60*60*4}, {"5 часов", 60*60*5},
						 {"6 часов", 60*60*6}, {"7 часов", 60*60*7}, {"8 часов", 60*60*8},  {"10 часов", 60*60*10},
						 {"12 часов", 60*60*12},  {"14 часов", 60*60*14},  {"16 часов", 60*60*16},
						 {"Ввести вручную", ForPostpone_ns::handInput},
						 };
	else QMbError("wrong menuPostponeCaseValue");

	std::set<Note*> notesToDo { note };
	if(note == NoteForPostponeAll())
	{
		notesToDo.clear();
		for(auto &note:notes) notesToDo.emplace(note->note);
	}
	else if(note == NoteForPostponeSelected())
	{
		notesToDo = GetSelectedNotes();
	}

	for(auto &delay:delays)
	{
		int delaySecs = delay.seconds;

		if(delay.seconds == ForPostpone_ns::separator)
		{
			menu->addSeparator();
			continue;
		}

		if(delay.seconds != ForPostpone_ns::handInput)
		{
			if(menuPostponeCaseCurrent == menuPostponeCase::setPostpone)
			{
				delay.text += AddSecsFromNow(delaySecs).toString(" (hh:mm::ss)");
			}
			else if(menuPostponeCaseCurrent == menuPostponeCase::changeDtNotify)
			{
				if(notesToDo.size() == 1) // если обрабатываеся одна зазача
				{
					delay.text = AddSecsFromToday((*notesToDo.begin())->DTNotify(), delaySecs).toString("dd MMM yyyy hh:mm::ss (ddd)");
					if(delaySecs == secondsInDay)
						delay.text = AddSecsFromToday((*notesToDo.begin())->DTNotify(), delaySecs).toString("завтра hh:mm::ss (ddd)");
					if(delaySecs == secondsInDay*2)
						delay.text = AddSecsFromToday((*notesToDo.begin())->DTNotify(), delaySecs).toString("послезавтра hh:mm::ss (ddd)");
				}
				else // если обрабатываеся много зазач
				{
					delay.text = QDateTime::currentDateTime().addSecs(delaySecs).toString("dd MMM yyyy (ddd)");
					if(delaySecs == secondsInDay)
						delay.text = "завтра";
					if(delaySecs == secondsInDay*2)
						delay.text = "послезавтра";
				}
			}
		}

		menu->addAction(delay.text);
		connect(menu->actions().back(), &QAction::triggered, [this, delaySecs, notesToDo](){
			SlotPostpone(notesToDo, delaySecs, menuPostponeCaseCurrent);
		});

		// добавление разделителей
		if(menu->actions().back()->text().contains("(Вс)"))
			menu->addSeparator();
	}

	menu->exec(pos);
}

void WidgetAlarms::SlotPostpone(std::set<Note*> notesToPostpone, int delaySecs, menuPostponeCase caseCurrent)
{
	if(delaySecs == ForPostpone_ns::handInput)
	{
		auto res = DialogInputTime::Execute();
		if(!res.accepted) return;
		delaySecs = DialogInputTime::TotalSecs(res);
	}

	// чтобы после 12 ночи случайно не перенести заметку на "завтра" или "послезавта", хотя это будет на день дальше
	auto currentTime = QTime::currentTime();
	std::function<bool(QDateTime newDt)> Question;
	QTime morningStart(0,0,0), morningEnd(8,0,0);
	// morningStart = QTime(18,0,0); morningEnd = QTime(19,0,0); // for testing
	if(caseCurrent == menuPostponeCase::changeDtNotify
			and delaySecs <= secondsInDay*2
			and currentTime >= morningStart and currentTime < morningEnd)
		Question = [](QDateTime newDt) -> bool
		{
			auto answ = QMessageBox::question({}, "Date change",
								"It's already the morning of the "+QDateTime::currentDateTime().toString(DateTimeFormat_rus)
								+ ".\n\nDo you really want to move the task to the "+newDt.toString(DateTimeFormat_rus));
			if(answ == QMessageBox::Yes) return true;
			else if(answ == QMessageBox::No) {}
			else QMbError("Unrealesed answ");
			return false;
		};

	for(auto &noteToPospone:notesToPostpone)
	{
		if(caseCurrent == menuPostponeCase::changeDtNotify)
		{
			auto newTime = AddSecsFromToday(noteToPospone->DTNotify(), delaySecs);
			if(Question and Question(newTime) == false)
				return;
			noteToPospone->SetDT(newTime, newTime);
		}
		else if(caseCurrent == menuPostponeCase::setPostpone)
		{
			auto newTime = AddSecsFromNow(delaySecs);
			noteToPospone->SetDTPostpone(newTime);
		}

		if(notes.empty()) hide();
	}
}

Note *WidgetAlarms::NoteOfCurrentRow(bool getFirstIfNoSelected)
{
	int index = table->currentRow();
	if(index == -1)
	{
		if(getFirstIfNoSelected)
			index = 0;
		else return nullptr;
	}

	if(index >= 0 and index < (int)notes.size())
		return notes[index]->note;

	QMbError("btnFastActions: bad index");
	return nullptr;
}

std::set<Note *> WidgetAlarms::GetSelectedNotes()
{
	std::set<Note*> notesSet;
	auto ranges = table->selectedRanges();
	for(auto &range:ranges)
	{
		for(int r = range.topRow(); r<=range.bottomRow(); r++)
			notesSet.emplace(notes[r]->note);
	}
	return notesSet;
}

void WidgetAlarms::showEvent(QShowEvent * event)
{
	if(!QFile::exists(settingsFile)) return;

	QSettings settings(settingsFile, QSettings::IniFormat);
	auto geo = settings.value("geo").toByteArray();
	if(settings.contains("geo") && !geo.isNull() && !geo.isEmpty())
	{
		restoreGeometry(geo);

//		auto geoRect = geometry();
//		QString geoStr = "X:" + QSn(geoRect.x()) + " Y: " + QSn(geoRect.y())
//				 + " Width: " + QSn(geoRect.width()) + " Height: " + QSn(geoRect.height()) + "\n";
//		QString log = QDateTime::currentDateTime().toString(DateTimeFormat) +
//							" WidgetAlarms::showEvent restored geo: " + geoStr + "\n";
//		qdbg << " WidgetAlarms::showEvent restored geo: " + geoStr + "\n";
//		MyQFileDir::AppendFile(QFileInfo(settingsFile).path() + "/save geo log.txt", log);
	}
	else
	{
		setGeometry(30,30, 640,300);
		if(!settings.contains("geo")) QMbError("!settings.contains(\"geo\")");
		if(geo.isNull()) QMbError("geo.isNull()");
		if(geo.isEmpty()) QMbError("geo.isEmpty()");
	}

	QTimer::singleShot(10,this,[this]{ resize(size()+QSize(1,1)); resize(size()-QSize(1,1)); });
										/// Экспериментально подобранный костыль чтобы лейбл с именем получил нужную ширину.
										/// Несмотря на то что resizeEvent все что делает - FitColWidth,
										/// тут вызова FitColWidth не достаточно. Ненавижу работу с размерами виджетов Qt !!!!!!!!!!

	event->accept();
}

void WidgetAlarms::FitColWidth()
{
	int columnCount = table->columnCount();

	if (columnCount != 1)
	{
		static bool preinted = false;
		if(!preinted) { preinted = true; QMbError("resizeEvent wrong columnCount"); }

		return;
	}

	int columnWidth = table->width() - 5;
	if(table->verticalScrollBar()->isVisible()) columnWidth -= table->verticalScrollBar()->width();
	table->setColumnWidth(0, columnWidth);

	//for(auto &note:notes) SetLabelText(*note.get());
}

void WidgetAlarms::InitFitColWidthTimer()
{
	auto timer = new QTimer(this);
	timer->start(10);
	connect(timer, &QTimer::timeout, this, [this](){
		if(fitColWidthRequest)
		{
			FitColWidth();
			fitColWidthRequest = false;
		}
	});
}

void WidgetAlarms::InitTimerSetterLabels()
{
	static bool inited = false;
	if(inited) { QMbError("multiple call InitThreadSetterLabes"); return; }

	QTimer *timer = new QTimer(this);
	timer->start(10);
	connect(timer, &QTimer::timeout, this, [this](){
		if(!notesToSetLabel.empty() and setLabelRequestDt.msecsTo(QDateTime::currentDateTime()) > 30)
		{
			for(auto &note:notesToSetLabel)
			{
				if(NoteInAlarms::validNotesInAlarms.count(note) > 0)
					SetLabelText(*note);
			}
			notesToSetLabel.clear();
		}
	});
}

void WidgetAlarms::InitMessageForNotifyTimer()
{
	auto timer = new QTimer(this);
	timer->start(10);

	connect(timer, &QTimer::timeout, this, [this](){
		while(!notesToShowMessageForNotify.empty())
		{
			Note *note = notesToShowMessageForNotify.front();

			QString text = "Message-notify for note:\n" + note->Name();
			QStringList buttons {"Open for edit", "Move up in notify widget", "Nothing to do"};
			if(notesToShowMessageForNotify.size() > 1)
			{
				buttons.insert(2, "Move up all");
				buttons.append("Nothing for all");

				text += "\n\nand than going messages for "+QSn(notesToShowMessageForNotify.size()-1)+" notes: ";
				for(uint i=1; i<notesToShowMessageForNotify.size(); i++)
				{
					if(i >= 6) { text += "\n..."; break; }
					text += "\n"+notesToShowMessageForNotify[i]->Name();
				}
			}

			auto answ = MyQDialogs::CustomDialog("Message-notify for note", text, buttons);

			bool toAll = false;

			if(answ == "Open for edit"){
				WidgetNoteEditor::MakeOrShowNoteEditor(*note);
			}
			else if(answ == "Move up in notify widget") {
				MoveNoteUp(*note);
			}
			else if(answ == "Move up all")
			{
				toAll = true;
				for(auto &note:notesToShowMessageForNotify) MoveNoteUp(*note);
			}
			else if(answ == "Nothing to do"){

			}
			else if(answ == "Nothing for all"){
				toAll = true;
			}
			else QMbError("Unrealesed answ");

			if(toAll) { notesToShowMessageForNotify.clear(); break; }

			notesToShowMessageForNotify.erase(notesToShowMessageForNotify.begin());
		}
	});
}

void WidgetAlarms::resizeEvent(QResizeEvent * event)
{
	QWidget::resizeEvent(event);
	FitColWidth();
	for(auto &note:notes) notesToSetLabel.push_back(note.get());
	setLabelRequestDt = QDateTime::currentDateTime();
}

