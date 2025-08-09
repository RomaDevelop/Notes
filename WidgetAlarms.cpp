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

const QString& RescheduleCaption() { static QString str = " Перенести на ... "; return str; }
const QString& RostponeCaption() { static QString str = " Отложить на ... "; return str; }
const QString& RemoveCaption() { static QString str = " Удалить "; return str; }
const QString& AllButtonsCaptions() {
	static QString str = QString(RescheduleCaption()).append(RostponeCaption()).append(RemoveCaption()); return str; }

WidgetAlarms::WidgetAlarms(INotesOwner *aNotesOwner, QFont fontForLabels, QWidget *parent):
	QWidget(parent),
	fontForLabels{fontForLabels},
	fontMetrixForLabels(fontForLabels),
	notesOwner {aNotesOwner}
{
	settingsFile = MyQDifferent::PathToExe() + "/files/settings_widget_alarms.ini";

	setWindowFlag(Qt::WindowCloseButtonHint, false);
	setWindowFlag(Qt::WindowDoesNotAcceptFocus, true);

	QVBoxLayout *vlo_main = new QVBoxLayout(this);
	QHBoxLayout *hlo1 = new QHBoxLayout;
	QHBoxLayout *hlo2 = new QHBoxLayout;
	vlo_main->addLayout(hlo1);
	vlo_main->addLayout(hlo2);

	table = new QTableWidget;
	table->setColumnCount(1);
	table->verticalHeader()->hide();
	table->horizontalHeader()->hide();
	hlo1->addWidget(table);
	connect(table, &QTableWidget::cellDoubleClicked, [this](int r, int){
		WidgetNoteEditor::MakeOrShowNoteEditor(*notes[r]->note);
	});

	CreateBottomRow(hlo2);

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

void WidgetAlarms::GiveNotes(const std::vector<Note *> & givingNotes)
{
	bool added = false;
	for(uint i=0; i<notes.size();)
	{
		if(std::find(givingNotes.begin(), givingNotes.end(), notes[i]->note) == givingNotes.end())
			RemoveNoteFromWidgetAlarms(i);
		else ++i;
	}

	for(auto &newNote:givingNotes)
	{
		if(NoteInAlarms *findedNote = FindNote(newNote); findedNote == nullptr)
		{
			AddNote(newNote, true);
			added = true;
		}
	}

	setWindowTitle(QSn(notes.size()) + " alarms for notes");

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

void WidgetAlarms::CreateBottomRow(QHBoxLayout *hlo)
{
	auto btnShowMainWindow = new QToolButton();
	btnShowMainWindow->setIcon(QIcon(Resources::list().GetPathName()));
	hlo->addWidget(btnShowMainWindow);
	connect(btnShowMainWindow, &QPushButton::clicked, this, [this](){ notesOwner->ShowMainWindow(); });

	auto btnMostOpenedNotes = new QToolButton();
	btnMostOpenedNotes->setIcon(QIcon(Resources::list_mo().GetPathName()));
	hlo->addWidget(btnMostOpenedNotes);
	connect(btnMostOpenedNotes, &QPushButton::clicked, this, [this](){ notesOwner->MostOpenedNotes(); });

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

	hlo->addStretch();

	auto btnRescheduleSelected = new QPushButton(" Перенести выбранные на ... ");
	auto btnPostponeSelected = new QPushButton(" Отложить выбранные на ... ");
	btnRescheduleSelected->hide();
	btnPostponeSelected->hide();
	hlo->addWidget(btnRescheduleSelected);
	hlo->addWidget(btnPostponeSelected);
	connect(btnRescheduleSelected, &QPushButton::clicked, [this, btnRescheduleSelected](){
		ShowMenuPostpone(btnRescheduleSelected->mapToGlobal(QPoint(0, btnRescheduleSelected->height())),
						 changeDtNotify, NoteForPostponeSelected());
	});

	connect(btnPostponeSelected, &QPushButton::clicked, [this, btnPostponeSelected](){
		ShowMenuPostpone(btnPostponeSelected->mapToGlobal(QPoint(0, btnPostponeSelected->height())),
						 setPostpone, NoteForPostponeSelected());
	});
	connect(table, &QTableWidget::itemSelectionChanged, this, [this, btnRescheduleSelected, btnPostponeSelected](){
		if(GetSelectedNotes().size() > 1)
			{ btnRescheduleSelected->show(); btnPostponeSelected->show(); }
		else { btnRescheduleSelected->hide(); btnPostponeSelected->hide(); }
	});

	hlo->addSpacing(10);

	auto btnRescheduleAll = new QPushButton(" Перенести все на ... ");
	auto btnPostponeAll = new QPushButton(" Отложить все на ... ");
	hlo->addWidget(btnRescheduleAll);
	hlo->addWidget(btnPostponeAll);
	connect(btnRescheduleAll, &QPushButton::clicked, [this, btnRescheduleAll](){
		ShowMenuPostpone(btnRescheduleAll->mapToGlobal(QPoint(0, btnRescheduleAll->height())),
						 changeDtNotify, NoteForPostponeAll());
	});

	connect(btnPostponeAll, &QPushButton::clicked, [this, btnPostponeAll](){
		ShowMenuPostpone(btnPostponeAll->mapToGlobal(QPoint(0, btnPostponeAll->height())),
						 setPostpone, NoteForPostponeAll());
	});
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
		if(!newNoteInAlarmsPtr->note->CheckAlarm(QDateTime::currentDateTime()))
		{
			RemoveNoteFromWidgetAlarms(newNoteInAlarmsPtr->note, true);
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
			RemoveNoteFromWidgetAlarms(note, true);
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
	RemoveNoteFromWidgetAlarms(&note, true);
	AddNote(&note, true, true);
}

void WidgetAlarms::SetLabelText(NoteInAlarms & note)
{
	QString text1 = "   " + note.note->Name();
	QString text2 = "  ("+note.note->DTNotify().toString("dd MMM yyyy hh:mm:ss")+")";
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

void WidgetAlarms::RemoveNoteFromWidgetAlarms(int index)
{
	notes[index]->note->RemoveCbs(notes[index].get(), notes[index]->cbCounter);
	table->removeRow(index);
	notes.erase(notes.begin() + index);

	QTimer::singleShot(10,this,[this]{ FitColWidth(); });
}

void WidgetAlarms::RemoveNoteFromWidgetAlarms(Note * aNote, bool showError)
{
	for(uint i=0; i<notes.size(); i++)
		if(notes[i]->note == aNote)
		{
			RemoveNoteFromWidgetAlarms(i);
			return;
		}
	if(showError) QMbError("RemoveNoteFromWidgetAlarms: note " + aNote->Name() + " not found");
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
	const int secondsInDay = 60*60*24;
	const int daysInMonth = QDate::currentDate().daysInMonth();

	if(menuPostponeCaseCurrent == menuPostponeCase::changeDtNotify)
		delays = 		{{"1 день", secondsInDay}, {"2 дня", secondsInDay*2}, {"3 дня", secondsInDay*3}, {"4 дня", secondsInDay*4},
						 {"5 дней", secondsInDay*5}, {"6 дней", secondsInDay*6}, {"7 дней", secondsInDay*7}, {"8 дней", secondsInDay*8},
						 {"9 дней", secondsInDay*9}, {"10 дней", secondsInDay*10}, {"11 дней", secondsInDay*11}, {"12 дней", secondsInDay*12},
						 {"13 дней", secondsInDay*13}, {"14 дней", secondsInDay*14}, {"15 дней", secondsInDay*15},
						 {"", ForPostpone_ns::separator}, {"18 дней", secondsInDay*18},
						 {"21 дней", secondsInDay*21}, {"25 дней", secondsInDay*25}, {"Месяц", secondsInDay*daysInMonth},
						 {"", ForPostpone_ns::separator},
						 {"40 дней", secondsInDay*40}, {"50 дней", secondsInDay*50}, {"Два месяца", secondsInDay*daysInMonth*2},
						 {"Ввести вручную", ForPostpone_ns::handInput}};
	else if(menuPostponeCaseCurrent == menuPostponeCase::setPostpone)
		delays = 		{{"5 минут", 60*5}, {"10 минут", 60*10}, {"15 минут", 60*15}, {"20 минут", 60*20},
						 {"25 минут", 60*25}, {"30 минут", 60*30}, {"35 минут", 60*35}, {"40 минут", 60*40},
						 {"45 минут", 60*45}, {"50 минут", 60*50}, {"1 час", 60*60}, {"1,5 часа", 60*90},
						 {"2 часа", 60*60*2}, {"3 часа", 60*60*3}, {"4 часа", 60*60*4}, {"5 часов", 60*60*5},
						 {"6 часов", 60*60*6}, {"7 часов", 60*60*7}, {"8 часов", 60*60*8},  {"10 часов", 60*60*10},
						  {"12 часов", 60*60*12},  {"14 часов", 60*60*14},  {"16 часов", 60*60*16},
						 {"Ввести вручную", ForPostpone_ns::handInput}};
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
	int itogDelaySecs = delaySecs; // почему то не давал изменять значение delaySecs внутри лямбды
	if(itogDelaySecs == ForPostpone_ns::handInput)
	{
		auto res = MyQDialogs::InputLineExt("Введите значение", "Введите значение", "",
											{"Секунд","Минут","Часов","Дней","Отмена"}, 500);
		if(res.text.isEmpty()) return;
		if(!IsUInt(res.text)) { QMbError("Input is not number" + res.text); return; }
		if(0) ;
		else if(res.button == "Секунд") itogDelaySecs = res.text.toUInt();
		else if(res.button == "Минут") itogDelaySecs = res.text.toUInt()*60;
		else if(res.button == "Часов") itogDelaySecs = res.text.toUInt()*60*60;
		else if(res.button == "Дней") itogDelaySecs = res.text.toUInt()*60*60*24;
		else if(res.button == "Отмена") return;
		else if(res.button.isEmpty()) return;
		else QMbError("Error button name " + res.button);
	}

	for(auto &noteToPospone:notesToPostpone)
	{
		if(caseCurrent == menuPostponeCase::setPostpone)
		{
			noteToPospone->SetDT(noteToPospone->DTNotify(), AddSecsFromNow(itogDelaySecs));
		}
		else if(caseCurrent == menuPostponeCase::changeDtNotify)
		{
			noteToPospone->SetDT(AddSecsFromToday(noteToPospone->DTNotify(), itogDelaySecs), noteToPospone->DTNotify());
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

			bool toAll = false;
			bool *toAllPtr = nullptr;
			if(notesToShowMessageForNotify.size() > 1)
				toAllPtr = &toAll;
			auto answ = MyQDialogs::CustomDialog2("Message-notify for note", "Message-notify for note:\n" + note->Name(),
												 {"Open for edit", "Move up in notify widget", "Nothing to do"},
												 toAllPtr, "Apply to next "+QSn(notesToShowMessageForNotify.size())+" notes");

			std::vector<Note*> notesToDoNow;
			if(toAll) notesToDoNow = notesToShowMessageForNotify;
			else notesToDoNow = {note};

			if(answ == "Open for edit"){
				for(auto &note:notesToDoNow) WidgetNoteEditor::MakeOrShowNoteEditor(*note);
			}
			else if(answ == "Move up in notify widget") {
				for(auto &note:notesToDoNow) MoveNoteUp(*note);
			}
			else if(answ == "Nothing to do"){

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

