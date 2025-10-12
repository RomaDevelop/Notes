#include "Settings.h"

#include <QTime>
#include <QMessageBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

#include "MyQShortings.h"

QString Settings::SaveToString()
{
	return QSn(AlarmsJoinEnabled)+"|"+QSn(AlarmsJoinMaxSecs);
}

void Settings::LoadFromString(const QString & str)
{
	auto list = str.splitRef("|");
	if(list.size() != 2)
	{
		QMbError("LoadFromString list.size() != 2");
		return;
	}

	AlarmsJoinEnabled = list[0].toInt();
	AlarmsJoinMaxSecs = list[1].toInt();
}

void DialogSettings::Execute()
{
	DialogSettings dialog;
	dialog.exec();
}

DialogSettings::DialogSettings() : QDialog()
{
	setWindowTitle("Notes: settings");

	auto gloMain = new QGridLayout(this);

	chBoxAlarmsJoinEnabled = new QCheckBox;
	leAlarmsJoinMaxSecs = new QLineEdit;
	auto labelAlarmsJoinMaxTime = new QLabel;
	auto buttonOk = new QPushButton("Ok");
	auto buttonCancel = new QPushButton("Cansel");

	chBoxAlarmsJoinEnabled->setChecked(Settings::AlarmsJoinEnabled);
	leAlarmsJoinMaxSecs->setText(QSn(Settings::AlarmsJoinMaxSecs));
	labelAlarmsJoinMaxTime->setText(QTime(0,0,0).addSecs(Settings::AlarmsJoinMaxSecs).toString(TimeFormat));
	connect(leAlarmsJoinMaxSecs, &QLineEdit::textChanged, this, [this, labelAlarmsJoinMaxTime](){
		labelAlarmsJoinMaxTime->setText(QTime(0,0,0).addSecs(leAlarmsJoinMaxSecs->text().toUInt()).toString(TimeFormat));
	});

	int row=0;
	gloMain->addWidget(new QLabel("Alarms join enabled:"), row, 0);
	gloMain->addWidget(chBoxAlarmsJoinEnabled, row, 1);

	row++;
	gloMain->addWidget(new QLabel("Alarms join max secs:"), row, 0);
	gloMain->addWidget(leAlarmsJoinMaxSecs, row, 1);
	gloMain->addWidget(labelAlarmsJoinMaxTime, row, 2);

	auto hloButtons = new QHBoxLayout;

	row++;
	gloMain->addLayout(hloButtons, row, 0, 1, -1);

	hloButtons->addStretch();
	hloButtons->addWidget(buttonOk);
	hloButtons->addWidget(buttonCancel);

	connect(buttonOk, &QPushButton::clicked, this, [this](){
		bool ok;
		auto newSecsValue = leAlarmsJoinMaxSecs->text().toUInt(&ok);
		if(!ok)
		{
			QMbError("Invalid value "+leAlarmsJoinMaxSecs->text());
			return;
		}

		Settings::AlarmsJoinEnabled = chBoxAlarmsJoinEnabled->isChecked();
		Settings::AlarmsJoinMaxSecs = newSecsValue;

		close();
	});
	connect(buttonCancel, &QPushButton::clicked, this, [this](){
		close();
	});
}
