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
#include "declare_struct.h"

#include "FastActions.h"
#include "WidgetNoteEditor.h"
#include "Resources.h"

WidgetAlarms::WidgetAlarms(QFont fontForLabels,
						   std::function<void()> crNewNoteFoo,
						   std::function<void()> showMainWindow,
						   QWidget * parent):
	QWidget(parent),
	fontForLabels{fontForLabels},
	fontMetrixForLabels(fontForLabels)
{
	settingsFile = MyQDifferent::PathToExe() + "/files/settings_widget_alarms.ini";

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
		WidgetNoteEditor::MakeOrShowNoteEditor(*notes[r]->note);
	});

	auto btnShowMainWindow = new QToolButton();
	btnShowMainWindow->setIcon(QIcon(Resources::list().GetPathName()));
	hlo2->addWidget(btnShowMainWindow);
	connect(btnShowMainWindow, &QPushButton::clicked, showMainWindow);

	auto btnAdd = new QToolButton();
	btnAdd->setIcon(QIcon(Resources::add().GetPathName()));
	hlo2->addWidget(btnAdd);
	connect(btnAdd, &QPushButton::clicked, crNewNoteFoo);

	auto btnFastActions = new QToolButton();
	btnFastActions->setIcon(QIcon(Resources::action().GetPathName()));
	hlo2->addWidget(btnFastActions);
	connect(btnFastActions, &QPushButton::clicked, [this, btnFastActions](){
		int index = table->currentRow();
		notes[index]->note->ShowDialogFastActions(btnFastActions);
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
			AddNote(newNote);
			added = true;
		}
	}

	setWindowTitle(QSn(notes.size()) + " alarms for notes");

	if(!notes.empty())
	{
		show();
		if(added)
			this->windowHandle()->alert(5000);
	}
	else hide();
}

NoteInAlarms * WidgetAlarms::FindNote(Note * noteToFind)
{
	for(auto &note:notes)
		if(note->note == noteToFind) return note.get();
	return nullptr;
}

void WidgetAlarms::AddNote(Note * note)
{
	table->setRowCount(table->rowCount()+1);
	int row = table->rowCount()-1;
	auto widget = new QWidget;
	table->setCellWidget(row, 0, widget);

	auto hlo = new QHBoxLayout(widget);
	hlo->setContentsMargins(0,0,0,0);
	auto labelCaption1 = new QLabel;
	labelCaption1->setFont(fontForLabels);
	labelCaption1->setMaximumWidth(table->width() - 480);
	auto labelCaption2 = new QLabel;
	labelCaption2->setFont(fontForLabels);
	hlo->addWidget(labelCaption1);
	hlo->addWidget(labelCaption2);
	hlo->addStretch();
	auto btnReschedule = new QPushButton(" Перенести на ... ");
	auto btnPostpone = new QPushButton(" Отложить на ... ");
	auto btnRemove = new QPushButton(" Удалить ");
	hlo->addWidget(btnReschedule);
	hlo->addWidget(btnPostpone);
	hlo->addWidget(btnRemove);
	hlo->addSpacing(4);

	NoteInAlarms *newNoteInAlarmsPtr = notes.emplace_back(new NoteInAlarms).get();
	NoteInAlarms &newNoteInAlarms = *newNoteInAlarmsPtr;
	newNoteInAlarms.note = note;
	newNoteInAlarms.labelCaption1 = labelCaption1;
	newNoteInAlarms.labelCaption2 = labelCaption2;

	SetLabelText(newNoteInAlarms);

	auto cb = [this, newNoteInAlarmsPtr](void*){ SetLabelText(*newNoteInAlarmsPtr); };
	note->AddCBNameUpdated(cb, newNoteInAlarmsPtr, newNoteInAlarmsPtr->cbCounter);
	note->AddCBDTUpdated(cb, newNoteInAlarmsPtr, newNoteInAlarmsPtr->cbCounter);

	connect(btnReschedule, &QPushButton::clicked, [this, btnReschedule, note](){
		ShowMenuPostpone(btnReschedule->mapToGlobal(QPoint(0, btnReschedule->height())), changeDtNotify, note);
	});
	connect(btnPostpone, &QPushButton::clicked, [this, btnPostpone, note](){
		ShowMenuPostpone(btnPostpone->mapToGlobal(QPoint(0, btnPostpone->height())), setPostpone, note);
	});
	connect(btnRemove, &QPushButton::clicked, [note](){
		if(QMessageBox::question(0,"Remove note","Removing note "+note->Name()+"\n\nAre you shure?") == QMessageBox::Yes)
			note->RemoveNoteFromBase();
	});

	QTimer::singleShot(10,this,[this]{ FitColWidth(); });
}

void WidgetAlarms::SetLabelText(NoteInAlarms & note)
{
	int labelCaption1W = table->width() - 440;
	if(labelCaption1W < 50) labelCaption1W = 50;
	note.labelCaption1->setMaximumWidth(labelCaption1W);

	QString text1 = "   " + note.note->Name();
	if(fontMetrixForLabels.boundingRect(text1).width() + 15 > labelCaption1W)
	{
		while(fontMetrixForLabels.boundingRect(text1).width() + 20 > labelCaption1W - fontMetrixForLabels.boundingRect("...").width())
			text1.chop(1);
		text1 += "...";
	}

	note.labelCaption1->setText(text1);
	note.labelCaption2->setText("("+note.note->DTNotify().toString("dd MMM yyyy hh:mm:ss")+")");
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
	if(showError) QMbError("RemoveNote: note " + aNote->Name() + " not found");
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

	if(menuPostponeCaseCurrent == menuPostponeCase::changeDtNotify)
		delays = 		{{"1 день", secondsInDay}, {"2 дня", secondsInDay*2}, {"3 дня", secondsInDay*3}, {"4 дня", secondsInDay*4},
						 {"5 дней", secondsInDay*5}, {"6 дней", secondsInDay*6}, {"7 дней", secondsInDay*7}, {"8 дней", secondsInDay*8},
						 {"9 дней", secondsInDay*9}, {"10 дней", secondsInDay*7}, {"10 дней", secondsInDay*10}, {"11 дней", secondsInDay*11},
						 {"12 дней", secondsInDay*12}, {"13 дней", secondsInDay*13}, {"14 дней", secondsInDay*14},
						 {"20 дней", secondsInDay*20}, {"25 дней", secondsInDay*25}, {"30 дней", secondsInDay*30},
						 {"40 дней", secondsInDay*40}, {"50 дней", secondsInDay*50}, {"60 дней", secondsInDay*60},
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

	std::vector<Note*> notesToDo { note };
	if(note == NoteForPostponeAll())
	{
		notesToDo.clear();
		for(auto &note:notes) notesToDo.emplace_back(note->note);
	}

	for(auto &delay:delays)
	{
		int delaySecs = delay.seconds;

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
					delay.text = AddSecsFromToday(notesToDo[0]->DTNotify(), delaySecs).toString("dd MMM yyyy hh:mm::ss (ddd)");
					if(delaySecs == secondsInDay)
						delay.text = AddSecsFromToday(notesToDo[0]->DTNotify(), delaySecs).toString("завтра hh:mm::ss (ddd)");
					if(delaySecs == secondsInDay*2)
						delay.text = AddSecsFromToday(notesToDo[0]->DTNotify(), delaySecs).toString("послезавтра hh:mm::ss (ddd)");
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

void WidgetAlarms::SlotPostpone(std::vector<Note*> notesToPostpone, int delaySecs, menuPostponeCase caseCurrent)
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

	for(uint i=0; i<notesToPostpone.size(); i++)
	{
		if(caseCurrent == menuPostponeCase::setPostpone)
		{
			notesToPostpone[i]->SetDT(notesToPostpone[i]->DTNotify(), AddSecsFromNow(itogDelaySecs));
		}
		else if(caseCurrent == menuPostponeCase::changeDtNotify)
		{
			notesToPostpone[i]->SetDT(AddSecsFromToday(notesToPostpone[i]->DTNotify(), itogDelaySecs), notesToPostpone[i]->DTNotify());
		}

		if(!notesToPostpone[i]->CheckAlarm(QDateTime::currentDateTime()))
			RemoveNoteFromWidgetAlarms(notesToPostpone[i], false);

		if(notes.empty()) hide();
	}
}

void WidgetAlarms::showEvent(QShowEvent * event)
{
	if(!QFile::exists(settingsFile)) return;

	QSettings settings(settingsFile, QSettings::IniFormat);
	auto geo = settings.value("geo").toByteArray();
	if(settings.contains("geo") && !geo.isNull() && !geo.isEmpty())
	{
		restoreGeometry(geo);

		auto geoRect = geometry();
		QString geoStr = "X:" + QSn(geoRect.x()) + " Y: " + QSn(geoRect.y())
				 + " Width: " + QSn(geoRect.width()) + " Height: " + QSn(geoRect.height()) + "\n";
		QString log = QDateTime::currentDateTime().toString(DateTimeFormat) +
							" WidgetAlarms::showEvent restored geo: " + geoStr + "\n";
		qdbg << " WidgetAlarms::showEvent restored geo: " + geoStr + "\n";
		MyQFileDir::AppendFile(QFileInfo(settingsFile).path() + "/save geo log.txt", log);
	}
	else
	{
		setGeometry(30,30, 640,300);
		if(!settings.contains("geo")) QMbError("!settings.contains(\"geo\")");
		if(geo.isNull()) QMbError("geo.isNull()");
		if(geo.isEmpty()) QMbError("geo.isEmpty()");
	}

	hasValidGeo = true;

	QTimer::singleShot(10,this,[this]{ FitColWidth(); });

	event->accept();
}

void WidgetAlarms::closeEvent(QCloseEvent * event)
{
	SaveSettings();
	event->accept();
}

void WidgetAlarms::SaveSettings()
{
	if(hasValidGeo)
	{
		QDir().mkpath(QFileInfo(settingsFile).path());
		MyQFileDir::WriteFile(settingsFile, "");
		QSettings settings(settingsFile, QSettings::IniFormat);

		auto geo = saveGeometry();
		settings.setValue("geo", geo);

		if (geo.size() < 16) {
			qDebug() << "Недостаточно данных для извлечения геометрии.";
			return;
		}

		QWidget newWidget;
		newWidget.restoreGeometry(geo);

		auto geoRect = geometry();
		QString geoStr = "X:" + QSn(geoRect.x()) + " Y: " + QSn(geoRect.y())
				 + " Width: " + QSn(geoRect.width()) + " Height: " + QSn(geoRect.height()) + "\n";
		QString log = QDateTime::currentDateTime().toString(DateTimeFormat) +
							" WidgetAlarms::SaveSettings saved geo: " + geoStr + "\n";
		qdbg << " WidgetAlarms::SaveSettings saved geo: " + geoStr + "\n";
		MyQFileDir::AppendFile(QFileInfo(settingsFile).path() + "/save geo log.txt", log);
	}
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

	for(auto &note:notes) SetLabelText(*note.get());
}

void WidgetAlarms::resizeEvent(QResizeEvent * event)
{
	QWidget::resizeEvent(event);
	FitColWidth();
}

