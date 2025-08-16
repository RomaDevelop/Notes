#ifndef DIALOGINPUTTIME_H
#define DIALOGINPUTTIME_H

#include <QDialog>

#include "declare_struct.h"

class DialogInputTime : public QDialog
{
	Q_OBJECT
public:
	declare_struct_5_fields_no_move(Result, bool, accepted, uint, days, uint, hours, uint, minutes, uint, seconds);
	static Result Execute();
	static uint64_t TotalSecs(const Result &result);

private:
	explicit DialogInputTime(QWidget *parent = nullptr);
	Result result;

};

#endif // DIALOGINPUTTIME_H
