#include "WidgetNotifie.h"

#include <QVBoxLayout>
#include <QCloseEvent>
#include <QSettings>
#include <QLabel>
#include <QPushButton>
#include <QScrollBar>

#include "MyQShortings.h"
#include "MyQTableWidget.h"
#include "MyQFileDir.h"
#include "MyQDifferent.h"
#include "MyQDialogs.h"
#include "declare_struct.h"

#include "NoteEditor.h"

WidgetAlarms::WidgetAlarms(QWidget * parent)
	: QWidget(parent)
{
	settingsFile = MyQDifferent::PathToExe() + "/files/settings_widget_alarms.ini";

	setWindowTitle("Notes notify");
	setWindowFlag(Qt::WindowCloseButtonHint, false);

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
		NoteEditor::MakeNoteEditor(*notes[r]);
	});

	hlo2->addStretch();
	auto btnReschedule = new QPushButton(" Перенести все на ... ");
	auto btnPostpone = new QPushButton(" Отложить все на ... ");
	hlo2->addWidget(btnReschedule);
	hlo2->addWidget(btnPostpone);
	connect(btnReschedule, &QPushButton::clicked, [this, btnReschedule](){
		ShowMenuPostpone(btnReschedule->mapToGlobal(QPoint(0, btnReschedule->height())), changeDtNotify, NoteForPostponeAll());
	});

	connect(btnPostpone, &QPushButton::clicked, [this, btnPostpone](){
		ShowMenuPostpone(btnPostpone->mapToGlobal(QPoint(0, btnPostpone->height())), setPostpone, NoteForPostponeAll());
	});
}

WidgetAlarms::~WidgetAlarms()
{
	SaveSettings();
}

void WidgetAlarms::GiveNotes(const std::vector<Note *> & givingNotes)
{
	for(uint i=0; i<notes.size();)
	{
		if(std::find(givingNotes.begin(), givingNotes.end(), notes[i]) == givingNotes.end())
			RemoveNote(i);
		else ++i;
	}

	for(auto &newNote:givingNotes)
	{
		if(std::find(notes.begin(), notes.end(), newNote) == notes.end())
			AddNote(newNote);
	}

	if(!notes.empty()) show();
	else hide();
}

void WidgetAlarms::AddNote(Note * note)
{
	notes.emplace_back(note);

	table->setRowCount(table->rowCount()+1);
	int row = table->rowCount()-1;
	auto widget = new QWidget;
	table->setCellWidget(row, 0, widget);

	auto hlo = new QHBoxLayout(widget);
	hlo->setContentsMargins(0,0,0,0);
	hlo->addWidget(new QLabel(GenLabelText(note)));
	hlo->addStretch();
	auto btnReschedule = new QPushButton(" Перенести на ... ");
	auto btnPostpone = new QPushButton(" Отложить на ... ");
	hlo->addWidget(btnReschedule);
	hlo->addWidget(btnPostpone);
	hlo->addSpacing(5);

	connect(btnReschedule, &QPushButton::clicked, [this, btnReschedule, note](){
		ShowMenuPostpone(btnReschedule->mapToGlobal(QPoint(0, btnReschedule->height())), changeDtNotify, note);
	});
	connect(btnPostpone, &QPushButton::clicked, [this, btnPostpone, note](){
		ShowMenuPostpone(btnPostpone->mapToGlobal(QPoint(0, btnPostpone->height())), setPostpone, note);
	});
}

QString WidgetAlarms::GenLabelText(Note * note)
{
	return "   " + note->name + " ("+note->dtNotify.toString("dd MMM yyyy hh:mm:ss")+")";
}

void WidgetAlarms::RemoveNote(int index)
{
	table->removeRow(index);
	notes.erase(notes.begin() + index);
}

void WidgetAlarms::RemoveNote(Note * aNote, bool showError)
{
	for(uint i=0; i<notes.size(); i++)
		if(notes[i] == aNote)
		{
			RemoveNote(i);
			return;
		}
	if(showError) QMbError("RemoveNote: note " + aNote->name + " not found");
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
	const int handInput = -1;
	if(menuPostponeCaseCurrent == menuPostponeCase::changeDtNotify)
		delays = 		{{"1 день", secondsInDay}, {"2 дня", secondsInDay*2}, {"3 дня", secondsInDay*3}, {"4 дня", secondsInDay*4},
						 {"5 дней", secondsInDay*5}, {"6 дней", secondsInDay*6}, {"7 дней", secondsInDay*7},
						 {"Ввести вручную", handInput}};
	else if(menuPostponeCaseCurrent == menuPostponeCase::setPostpone)
		delays = 		{{"5 минут", 60*5}, {"10 минут", 60*10}, {"15 минут", 60*15}, {"20 минут", 60*20},
						 {"25 минут", 60*25}, {"30 минут", 60*30}, {"35 минут", 60*35}, {"40 минут", 60*40},
						 {"45 минут", 60*45}, {"50 минут", 60*50}, {"1 час", 60*60}, {"1,5 часа", 60*90},
						 {"2 часа", 60*60*2}, {"3 часа", 60*60*3}, {"4 часа", 60*60*4}, {"5 часов", 60*60*5},
						 {"6 часов", 60*60*6}, {"7 часов", 60*60*7}, {"8 часов", 60*60*8}, {"Ввести вручную", handInput}};
	else QMbError("wrong menuPostponeCaseValue");

	std::vector<Note*> notesToTo { note };
	if(note == NoteForPostponeAll())
	{
		notesToTo = notes;
	}

	for(auto &delay:delays)
	{
		int delaySecs = delay.seconds;

		if(delay.seconds != handInput)
		{
			if(notesToTo.size() == 1) // если обрабатываеся одна зазача
			{
				if(menuPostponeCaseCurrent == menuPostponeCase::setPostpone)
				{
					delay.text += AddSecsFromNow(delaySecs).toString(" (hh:mm::ss)");
				}
				else if(menuPostponeCaseCurrent == menuPostponeCase::changeDtNotify)
				{
					delay.text = AddSecsFromToday(notesToTo[0]->dtNotify, delaySecs).toString("dd MMM yyyy hh:mm::ss (ddd)");
					if(delaySecs == secondsInDay)
						delay.text = AddSecsFromToday(notesToTo[0]->dtNotify, delaySecs).toString("завтра hh:mm::ss (ddd)");
					if(delaySecs == secondsInDay*2)
						delay.text = AddSecsFromToday(notesToTo[0]->dtNotify, delaySecs).toString("послезавтра hh:mm::ss (ddd)");
				}
			}
			else // если обрабатываеся много зазач
			{
				if(menuPostponeCaseCurrent == menuPostponeCase::setPostpone)
				{
					delay.text += AddSecsFromNow(delaySecs).toString(" (hh:mm::ss)");
				}
				else if(menuPostponeCaseCurrent == menuPostponeCase::changeDtNotify)
				{
					delay.text = QDateTime::currentDateTime().addSecs(delaySecs).toString("dd MMM yyyy (ddd)");
				}
			}
		}

		menu->addAction(new QAction(delay.text, menu));

		connect(menu->actions().back(), &QAction::triggered, [this, notesToTo, delaySecs](){
			int itogDelaySecs = delaySecs; // почему то не давал изменять значение delaySecs внутри лямбды
			if(itogDelaySecs == handInput)
			{
				auto res = MyQDialogs::InputLineExt("Введите значение", "", {"Секунд","Минут","Часов","Дней","Отмена"}, 500);
				if(res.line.isEmpty()) return;
				if(!IsUInt(res.line)) { QMbError("Input is not number" + res.line); return; }
				if(0) ;
				else if(res.button == "Секунд") itogDelaySecs = res.line.toUInt();
				else if(res.button == "Минут") itogDelaySecs = res.line.toUInt()*60;
				else if(res.button == "Часов") itogDelaySecs = res.line.toUInt()*60*60;
				else if(res.button == "Дней") itogDelaySecs = res.line.toUInt()*60*60*24;
				else if(res.button == "Отмена") return;
				else if(res.button.isEmpty()) return;
				else QMbError("Error button name " + res.button);
			}

			for(uint i=0; i<notesToTo.size(); i++)
			{
				if(menuPostponeCaseCurrent == menuPostponeCase::setPostpone)
					notesToTo[i]->dtPostpone = AddSecsFromNow(itogDelaySecs);
				else if(menuPostponeCaseCurrent == menuPostponeCase::changeDtNotify)
				{
					notesToTo[i]->dtNotify = AddSecsFromToday(notesToTo[i]->dtNotify, itogDelaySecs);
					notesToTo[i]->dtPostpone = notesToTo[i]->dtNotify;
				}
				if(!notesToTo[i]->CheckAlarm(QDateTime::currentDateTime()))
					RemoveNote(notesToTo[i], false);
				if(notes.empty()) hide();
				notesToTo[i]->EmitUpdated();
			}
		});
	}

	menu->exec(pos);
}

void WidgetAlarms::showEvent(QShowEvent * event)
{
	if(!QFile::exists(settingsFile)) return;

	QSettings settings(settingsFile, QSettings::IniFormat);
	restoreGeometry(settings.value("geo").toByteArray());
	event->accept();
}

void WidgetAlarms::closeEvent(QCloseEvent * event)
{
	SaveSettings();
	event->accept();
}

void WidgetAlarms::SaveSettings()
{
	QDir().mkpath(QFileInfo(settingsFile).path());
	MyQFileDir::WriteFile(settingsFile, "");
	QSettings settings(settingsFile, QSettings::IniFormat);

	settings.setValue("geo", saveGeometry());
}

void WidgetAlarms::resizeEvent(QResizeEvent * event)
{
	QWidget::resizeEvent(event);
	int columnCount = table->columnCount();

	if (columnCount != 1)
	{
		static bool preinted = false;
		if(!preinted) { preinted = true; QMbError("resizeEvent wrong columnCount"); }

		return;
	}

	int columnWidth = table->width();
	if(table->verticalScrollBar()->isVisible()) columnWidth -= table->verticalScrollBar()->width();
	table->setColumnWidth(0, columnWidth);
}

