#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <memory>

#include <QWidget>
#include <QTableWidget>
#include <QDateTime>

#include "Note.h"
#include "WidgetNotifie.h"

class MainWidget : public QWidget
{
	Q_OBJECT
public:
	QTableWidget *table;

	std::vector<std::unique_ptr<Note>> notes;
	WidgetAlarms widgetAlarms;

	explicit MainWidget(QWidget *parent = nullptr);
	~MainWidget() = default;

private:
	void CreateTrayIcon();
	void CreateNotesChecker();

	void closeEvent (QCloseEvent *event) override;
	//void resizeEvent(QResizeEvent * event) override { }
	//void moveEvent(QMoveEvent * event) override { }
	QString settingsFile;
	void SaveSettings();
	void LoadSettings();

	void CreateNotifyEditor(Note * noteToConnect, int rowIndex);
};


#endif // MAINWIDGET_H
