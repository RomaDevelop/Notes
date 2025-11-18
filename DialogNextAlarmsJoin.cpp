#include "DialogNextAlarmsJoin.h"

#include <QVBoxLayout>
#include <QDateTimeEdit>
#include <QMenu>

#include "MyQTableWidget.h"

#include "Note.h"
#include "Settings.h"

namespace Cols {
	const int name = 0;
	const int notify = name+1;
	const int postpone = notify+1;
	const int alarmIn = postpone+1;

	const int count = alarmIn+1;
}

void DialogNextAlarmsJoin::Execute(const std::vector<Note*> &notesToShow)
{
	if(notesToShow.empty()) { QMbInfo("No next alarms notes"); return; }
	auto dialog = DialogNextAlarmsJoin(notesToShow);
	dialog.exec();
}

DialogNextAlarmsJoin::DialogNextAlarmsJoin(const std::vector<Note*> &notesToShow):
	QDialog(),
	notes {notesToShow}
{
	resize(840,480);

	auto vloMain = new QVBoxLayout(this);

	CreateRow1(vloMain);

	table = new QTableWidget;
	vloMain->addWidget(table);
	table->setColumnCount(Cols::count);
	if(Settings::nextAlarmsNowTableHeaderState.get().isNull() == false)
		table->horizontalHeader()->restoreState(Settings::nextAlarmsNowTableHeaderState.get());

	table->setRowCount(notes.size());

	for(uint i=0; i<notes.size(); i++)
	{
		auto& note = notes[i];

		QString secsToAlarn = QTime(0,0,0).addSecs(
					note->SecsToAlarm(QDateTime::currentDateTime())).toString(TimeFormat);

		table->setItem(i, Cols::name, new QTableWidgetItem(note->Name()));
		table->setItem(i, Cols::notify, new QTableWidgetItem(note->DTNotifyStr()));
		table->setItem(i, Cols::postpone, new QTableWidgetItem(note->DTPostponeStr()));
		table->setItem(i, Cols::alarmIn, new QTableWidgetItem(secsToAlarn));
	}

	connect(table->horizontalHeader(), &QHeaderView::sectionResized, [this](){
		Settings::nextAlarmsNowTableHeaderState = table->horizontalHeader()->saveState();
	});
}

void DialogNextAlarmsJoin::CreateRow1(QVBoxLayout *vlo)
{
	auto hlo = new QHBoxLayout;
	vlo->addLayout(hlo);

	auto btnAlarmNow = new QPushButton("Alarm now");
	hlo->addWidget(btnAlarmNow);
	connect(btnAlarmNow, &QPushButton::clicked, [this](){
		auto rows = MyQTableWidget::SelectedRows(table,true);
		for(auto row:rows)
		{
			notes[row]->SetDTPostpone(QDateTime::currentDateTime());
		}
	});

	hlo->addStretch();
}
