#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <memory>

#include <QWidget>
#include <QTableWidget>
#include <QDateTime>

#include "Note.h"

class MainWidget : public QWidget
{
	Q_OBJECT
public:
	QTableWidget *table;

	std::vector<std::unique_ptr<Note>> notes;

	explicit MainWidget(QWidget *parent = nullptr);
	~MainWidget();

private:
	void closeEvent (QCloseEvent *event) override;
	//void resizeEvent(QResizeEvent * event) override { }
	//void moveEvent(QMoveEvent * event) override { }
	QString settingsFile;
	void SaveSettings();
	void LoadSettings();

	void CreateNotifyEditor(Note * noteToConnect, int rowIndex);
};
#endif // MAINWIDGET_H
