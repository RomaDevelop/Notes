#include "WindowClient.h"
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>

WidgetClient::WidgetClient(QWidget *parent) : QWidget(parent) {
	QVBoxLayout *vloMain = new QVBoxLayout(this);

	button = new QPushButton(" Send GET Request ");
	logEdit = new QTextEdit;
	logEdit->setReadOnly(true);

	QHBoxLayout *hlo1 = new QHBoxLayout;
	vloMain->addLayout(hlo1);
	QHBoxLayout *hlo2 = new QHBoxLayout;
	vloMain->addLayout(hlo2);

	hlo1->addWidget(button);
	connect(button, &QPushButton::clicked, this, &WidgetClient::SlotGet);

	auto button2 = new QPushButton(" Send POST Request ");
	hlo1->addWidget(button2);
	connect(button2, &QPushButton::clicked, this, &WidgetClient::SlotPost);

	leAddress = new QLineEdit("http://127.0.0.1:25002");
	hlo2->addWidget(leAddress);

	vloMain->addWidget(logEdit);

	resize(800,600);
	move(-950, 100);

	client.logFoo = [this](const QString &msg) {
		QMetaObject::invokeMethod(logEdit, [this, msg]() {
			logEdit->append(msg);
		});
	};
}

WidgetClient::~WidgetClient() {
	client.stop();
}

void WidgetClient::SlotGet() {
	QNetworkAccessManager *manager = new QNetworkAccessManager(this);

	connect(manager, &QNetworkAccessManager::finished, [this](QNetworkReply *reply) {
		if (reply->error()) {
			logEdit->append(reply->errorString());
		}
		else {
			logEdit->append(reply->readAll());
		}
		reply->deleteLater();
	});

	QNetworkRequest request(QUrl(leAddress->text()+"/1;2@3"));
	manager->get(request);

	QTimer::singleShot(1000, this, [this, manager]() {
		QNetworkRequest req2(QUrl(leAddress->text()+"/2&3&4 567"));
		manager->get(req2);
	});

}

void WidgetClient::SlotPost()
{
	QNetworkAccessManager *manager = new QNetworkAccessManager(this);

	connect(manager, &QNetworkAccessManager::finished, [this](QNetworkReply *reply) {
		if (reply->error()) {
			logEdit->append(reply->errorString());
		}
		else {
			logEdit->append(reply->readAll());
		}
		reply->deleteLater();
	});

	QNetworkRequest request(QUrl(leAddress->text()));
	request.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
	manager->post(request, "Очень длинная строка с множеством букав...");


	QTimer::singleShot(1000, this, [this, manager]() {
		QNetworkRequest request(QUrl(leAddress->text()));
		request.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
		manager->post(request, "Очень длинная строка с множеством букав... 2222");
	});

}
