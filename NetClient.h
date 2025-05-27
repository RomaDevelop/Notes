#ifndef NETCLIENT_H
#define NETCLIENT_H

#include <memory>

#include <QObject>
#include <QLayout>
#include <QTcpSocket>
#include <QTextEdit>
#include <QPushButton>
#include <QTimer>
#include <QCryptographicHash>

#include "declare_struct.h"

#include "NetConstants.h"

class NetClient : public QObject
{
	Q_OBJECT
public:
	explicit NetClient(QObject *parent = nullptr) : QObject(parent)
	{
		CreateWindowSocket(true);
		CreateSocket();
	}

	void Log(const QString &str);
	void Error(const QString &str);
	void Warning(const QString &str);

	std::unique_ptr<QWidget> widget;
	QTextEdit *textEditSocket;
	QTcpSocket *socket;
	bool canNetwork = false;
	int port = 25001;
	void CreateSocket();
	void CreateWindowSocket(bool show);
public slots:
	void SlotReadyRead();
	void SlotError(QAbstractSocket::SocketError err);

public:
	void SendToServer(QString str, bool sendEndMarker);
	void RequestToServer(const QString &requestType, QString content, std::function<void()> answWorker);
	void RequestsAnswersWorker(QString text);
	declare_struct_4_fields_move(RequestData, QString, id, QString, type, QString, content, QString, errors);
	static RequestData DecodeRequestCommand(QString command);
	static RequestData DecodeRequestAnswer(QString command);
	int TakeIdRequest() { return idRequest++; };
	int idRequest = 1;
	std::map<int, std::function<void()>> requestAswerWorkers; // int = id

	void SlotConnected();
};

#endif // NETCLIENT_H
