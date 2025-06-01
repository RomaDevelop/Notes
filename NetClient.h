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
#include <QLabel>
#include <QLineEdit>

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
	void SlotTest();

	void Log(const QString &str);
	void Error(const QString &str);
	void Warning(const QString &str);

	std::unique_ptr<QWidget> widget;
	QLineEdit *leArg;
	QTextEdit *textEditSocket;
	QTcpSocket *socket = nullptr;
	bool canNetwork = false;
	int port = 25001;
	void CreateSocket();
	void CreateWindowSocket(bool show);
public slots:
	void SlotReadyRead();
	void SlotError(QAbstractSocket::SocketError err);

public:
	void SendToServer(QString str, bool sendEndMarker);

	declare_struct_4_fields_move(RequestData, QString, id, QString, type, QString, content, QString, errors);
	using AnswerWorkerFunction = std::function<void(QString &&answContent)>;
	void RequestToServer(const QString &requestType, QString content, AnswerWorkerFunction answWorker);
	void RequestToServerWithWait(const QString &requestType, QString content, AnswerWorkerFunction answWorker);
	void RequestsAnswersWorker(QString text);
	static RequestData DecodeRequestCommand(QString command);
	static RequestData DecodeRequestAnswer(QString command);
	int TakeIdRequest() { return idRequest++; };
	int idRequest = 1;
	std::map<int, AnswerWorkerFunction> requestAswerWorkers; // int = id

	void SlotConnected();
};

#endif // NETCLIENT_H
