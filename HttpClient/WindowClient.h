#ifndef WINDOWCLIENT_H
#define WINDOWCLIENT_H

#include <QWidget>
#include <QLineEdit>

#include "HttpClient.h"

class QPushButton;
class QTextEdit;

class WidgetClient : public QWidget {
	Q_OBJECT

public:
	WidgetClient(QWidget *parent = nullptr);
	~WidgetClient();

private slots:
	void SlotGet();
	void SlotPost();

private:
	HttpClient client;
	QPushButton* button;
	QTextEdit* logEdit;
	QLineEdit *leAddress;
};

#endif // WINDOWCLIENT_H
