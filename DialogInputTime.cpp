#include "DialogInputTime.h"

#include <QVBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QRadioButton>
#include <QTimer>
#include <QMessageBox>
#include <QDateTime>

#include "MyQShortings.h"

DialogInputTimeResult DialogInputTime::Execute(QString caption, DialogInputTimeResult::from defaultFrom)
{
	DialogInputTime dialogVar(caption, defaultFrom);
	auto *dialog = &dialogVar;

	dialog->exec();

	return dialog->result;
}

DialogInputTime::DialogInputTime(QString caption, DialogInputTimeResult::from defaultFrom, QWidget *parent) : QDialog(parent)
{
	setWindowTitle(caption);
	setMinimumWidth(400);

	auto gloMain = new QGridLayout(this);

	leDays = new QLineEdit("0");
	leHours = new QLineEdit("0");
	leMinutes = new QLineEdit("0");
	leSeconds = new QLineEdit("0");

	rbtnFromToday = new QRadioButton("From today");
	rbtnFromToday->setToolTip("Notify time will stay same");
	rbtnFromNow = new QRadioButton("From now");
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

	labelResult = new QLabel;
	labelResult->setTextInteractionFlags(Qt::TextBrowserInteraction);

	gloMain->addWidget(new QLabel("Result"), ++row, 0);
	gloMain->addWidget(labelResult, row, 1);

	auto hloButtons = new QHBoxLayout;
	gloMain->addLayout(hloButtons, ++row, 0, 1, -1);

	hloButtons->addStretch();

	auto btnAccept = new QPushButton("Accept");
	hloButtons->addWidget(btnAccept);
	connect(btnAccept, &QPushButton::clicked, this, [this](){
		if(!CalcResult())
		{
			auto answ = QMessageBox::question({}, "Incorrect values", "Inputed values are incorrect. Use incorrect values as 0 and accept?");
			if(answ == QMessageBox::No) return ;
		}

		result.accepted = true;
		close();
	});

	auto btnCansel = new QPushButton("Cansel");
	hloButtons->addWidget(btnCansel);
	connect(btnCansel, &QPushButton::clicked, this, [this](){ close(); });

	connect(leDays,    &QLineEdit::textChanged, this, &DialogInputTime::CalcResult);
	connect(leHours,   &QLineEdit::textChanged, this, &DialogInputTime::CalcResult);
	connect(leMinutes, &QLineEdit::textChanged, this, &DialogInputTime::CalcResult);
	connect(leSeconds, &QLineEdit::textChanged, this, &DialogInputTime::CalcResult);

	connect(rbtnFromToday, &QRadioButton::clicked, this, &DialogInputTime::CalcResult);
	connect(rbtnFromNow,   &QRadioButton::clicked, this, &DialogInputTime::CalcResult);

	InitTimerCalcResult();
}

bool DialogInputTime::CalcResult()
{
	std::vector allEdits {leDays, leHours, leMinutes, leSeconds};
	for(auto &le:allEdits)
		if(!IsInt(le->text()))
		{
			labelResult->setText("incorrect input");
			return false;
		}

	result.days = leDays->text().toUInt();
	result.hours = leHours->text().toUInt();
	result.minutes = leMinutes->text().toUInt();
	result.seconds = leSeconds->text().toUInt();
	result.fromValue = rbtnFromToday->isChecked() ? DialogInputTimeResult::fromToday : DialogInputTimeResult::fromNow;

	result.totalSecs = 0;
	result.totalSecs += result.seconds;
	result.totalSecs += result.minutes*60;
	result.totalSecs += result.hours*60*60;
	result.totalSecs += result.days*60*60*24;

	if(result.fromValue == DialogInputTimeResult::fromToday)
	{
		int days = result.totalSecs / (24*60*60);
		auto destDay = QDateTime::currentDateTime().addDays(days);
		int secsToAddAfterDay = result.totalSecs - (days*24*60*60);
		QTime timeToAddAfterDay = QTime(0,0,0).addSecs(secsToAddAfterDay);

		labelResult->setText(destDay.toString(DateFormat_rus) + " + " + timeToAddAfterDay.toString(TimeFormat));
	}
	else // fromNow
	{
		auto now = QDateTime::currentDateTime();
		labelResult->setText(now.addSecs(result.totalSecs).toString(DateTimeFormat_rus));
	}

	return true;
}

void DialogInputTime::InitTimerCalcResult()
{
	timerCalcResult = new QTimer;
	connect(timerCalcResult, &QTimer::timeout, this, &DialogInputTime::CalcResult);
	timerCalcResult->start(1000);
}
