#include "DialogInputTime.h"

#include <QVBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>

#include "MyQShortings.h"

DialogInputTime::Result DialogInputTime::Execute()
{
	DialogInputTime dialogVar;
	auto *dialog = &dialogVar;

	dialog->exec();

	return dialog->result;
}

uint64_t DialogInputTime::TotalSecs(const Result &result)
{
	uint64_t secs = 0;
	secs += (uint64_t)result.seconds;
	secs += (uint64_t)result.minutes*60;
	secs += (uint64_t)result.hours*60*60;
	secs += (uint64_t)result.days*60*60*24;
	return secs;
}

DialogInputTime::DialogInputTime(QWidget *parent) : QDialog(parent)
{
	auto gloMain = new QGridLayout(this);

	auto leDays = new QLineEdit("0");
	auto leHours = new QLineEdit("0");
	auto leMinutes = new QLineEdit("0");
	auto leSeconds = new QLineEdit("0");

	int row = 0;
	gloMain->addWidget(new QLabel("Days"), row, 0);
	gloMain->addWidget(leDays, row, 1);

	gloMain->addWidget(new QLabel("Hours"), ++row, 0);
	gloMain->addWidget(leHours, row, 1);

	gloMain->addWidget(new QLabel("Minutes"), ++row, 0);
	gloMain->addWidget(leMinutes, row, 1);

	gloMain->addWidget(new QLabel("Seconds"), ++row, 0);
	gloMain->addWidget(leSeconds, row, 1);

	auto hlo = new QHBoxLayout;
	gloMain->addLayout(hlo, ++row, 0, 1, -1);

	hlo->addStretch();

	auto btnAccept = new QPushButton("Accept");
	hlo->addWidget(btnAccept);
	connect(btnAccept, &QPushButton::clicked, this, [this, leDays, leHours, leMinutes, leSeconds](){
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
		close();
	});

	auto btnCansel = new QPushButton("Cansel");
	hlo->addWidget(btnCansel);
	connect(btnCansel, &QPushButton::clicked, this, [this](){ close(); });
}
