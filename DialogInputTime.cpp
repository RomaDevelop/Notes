#include "DialogInputTime.h"

#include <QVBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QRadioButton>
#include <QMessageBox>

#include "MyQShortings.h"

DialogInputTimeResult DialogInputTime::Execute(DialogInputTimeResult::from defaultFrom)
{
	DialogInputTime dialogVar(defaultFrom);
	auto *dialog = &dialogVar;

	dialog->exec();

	return dialog->result;
}

uint64_t DialogInputTime::TotalSecs(const DialogInputTimeResult &result)
{
	uint64_t secs = 0;
	secs += (uint64_t)result.seconds;
	secs += (uint64_t)result.minutes*60;
	secs += (uint64_t)result.hours*60*60;
	secs += (uint64_t)result.days*60*60*24;
	return secs;
}

DialogInputTime::DialogInputTime(DialogInputTimeResult::from defaultFrom, QWidget *parent) : QDialog(parent)
{
	auto gloMain = new QGridLayout(this);

	auto leDays = new QLineEdit("0");
	auto leHours = new QLineEdit("0");
	auto leMinutes = new QLineEdit("0");
	auto leSeconds = new QLineEdit("0");

	auto rbtnFromToday = new QRadioButton("From today");
	rbtnFromToday->setToolTip("Notify time will stay same");
	auto rbtnFromNow = new QRadioButton("From now");
	rbtnFromNow->setToolTip("Notify time will be changed");

	if(defaultFrom == DialogInputTimeResult::fromToday)
		rbtnFromToday->setChecked(true);
	else if(defaultFrom == DialogInputTimeResult::fromNow)
		rbtnFromNow->setChecked(true);
	else { QMbError("DialogInputTimeResult::from"); }

	int row = 0;
	gloMain->addWidget(new QLabel("Days"), row, 0);
	gloMain->addWidget(leDays, row, 1);

	gloMain->addWidget(new QLabel("Hours"), ++row, 0);
	gloMain->addWidget(leHours, row, 1);

	gloMain->addWidget(new QLabel("Minutes"), ++row, 0);
	gloMain->addWidget(leMinutes, row, 1);

	gloMain->addWidget(new QLabel("Seconds"), ++row, 0);
	gloMain->addWidget(leSeconds, row, 1);

	auto vloFromChoose = new QVBoxLayout;
	gloMain->addLayout(vloFromChoose, ++row, 1, 1, -1);

	vloFromChoose->addWidget(rbtnFromToday);
	vloFromChoose->addWidget(rbtnFromNow);

	auto hloButtons = new QHBoxLayout;
	gloMain->addLayout(hloButtons, ++row, 0, 1, -1);

	hloButtons->addStretch();

	auto btnAccept = new QPushButton("Accept");
	hloButtons->addWidget(btnAccept);
	connect(btnAccept, &QPushButton::clicked, this, [this, leDays, leHours, leMinutes, leSeconds, rbtnFromToday](){
		std::vector allEdits {leDays, leHours, leMinutes, leSeconds};
		for(auto &le:allEdits)
			if(!IsInt(le->text()))
			{
				auto answ = QMessageBox::question({}, "Incorrect values", "Inputed values are incorrect. Use incorrect values as 0 and accept?");
				if(answ != QMessageBox::No) return;
			}

		result.accepted = true;
		result.days = leDays->text().toUInt();
		result.hours = leHours->text().toUInt();
		result.minutes = leMinutes->text().toUInt();
		result.seconds = leSeconds->text().toUInt();
		result.fromValue = rbtnFromToday->isChecked() ? DialogInputTimeResult::fromToday : DialogInputTimeResult::fromNow;
		close();
	});

	auto btnCansel = new QPushButton("Cansel");
	hloButtons->addWidget(btnCansel);
	connect(btnCansel, &QPushButton::clicked, this, [this](){ close(); });
}
