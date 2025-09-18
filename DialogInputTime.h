#ifndef DIALOGINPUTTIME_H
#define DIALOGINPUTTIME_H

#include <QDialog>

#include "declare_struct.h"

struct DialogInputTimeResult
{
	bool accepted;
	uint days;
	uint hours;
	uint minutes;
	uint seconds;

	enum from { fromToday, fromNow };
	from fromValue;
};

class DialogInputTime : public QDialog
{
	Q_OBJECT
public:
	static DialogInputTimeResult Execute(DialogInputTimeResult::from defaultFrom);
	static uint64_t TotalSecs(const DialogInputTimeResult &result);

private:
	explicit DialogInputTime(DialogInputTimeResult::from defaultFrom, QWidget *parent = nullptr);
	DialogInputTimeResult result;

};

#endif // DIALOGINPUTTIME_H
