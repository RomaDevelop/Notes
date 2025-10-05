#ifndef DIALOGINPUTTIME_H
#define DIALOGINPUTTIME_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QRadioButton>

#include "declare_struct.h"

struct DialogInputTimeResult
{
	bool accepted = false;
	uint64_t days;
	uint64_t hours;
	uint64_t minutes;
	uint64_t seconds;

	uint64_t totalSecs;

	enum from { fromToday, fromNow };
	from fromValue;
};

class DialogInputTime : public QDialog
{
	Q_OBJECT
public:
	static DialogInputTimeResult Execute(QString caption, DialogInputTimeResult::from defaultFrom);

private:
	explicit DialogInputTime(QString caption, DialogInputTimeResult::from defaultFrom, QWidget *parent = nullptr);
	DialogInputTimeResult result;

	QLineEdit *leDays;
	QLineEdit *leHours;
	QLineEdit *leMinutes;
	QLineEdit *leSeconds;

	QRadioButton *rbtnFromToday;
	QRadioButton *rbtnFromNow;

	QLabel *labelResult;
	bool CalcResult();
	void InitTimerCalcResult();
	QTimer *timerCalcResult;
};

#endif // DIALOGINPUTTIME_H
