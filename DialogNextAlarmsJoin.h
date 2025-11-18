#ifndef DIALOGNEXTALARMSJOIN_H
#define DIALOGNEXTALARMSJOIN_H

#include <QDialog>
#include <QTableWidget>
#include <QHBoxLayout>

struct Note;

class DialogNextAlarmsJoin : public QDialog
{
	Q_OBJECT
public:
	static void Execute(const std::vector<Note*> &notesToShow);

private:
	explicit DialogNextAlarmsJoin(const std::vector<Note*> &notesToShow);
	void CreateRow1(QVBoxLayout *vlo);

	const std::vector<Note*> &notes;

	QTableWidget *table;
};

#endif // DIALOGNEXTALARMSJOIN_H
