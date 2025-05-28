#include "NetClient.h"

#include <QProgressDialog>
#include <QCoreApplication>
#include <QPointer>
#include <QLabel>

#include "InputBlocker.h"

#include "MyQShortings.h"
#include "MyQTextEdit.h"
#include "MyCppDifferent.h"
#include "CodeMarkers.h"

void NetClient::SlotTest()
{
	Log("btnTestRequest pressed");


}

void NetClient::Log(const QString &str)
{
	textEditSocket->append(str);
}

void NetClient::Error(const QString &str)
{
	textEditSocket->append(str);
	MyQTextEdit::ColorizeLastCount(textEditSocket, Qt::red, str.size());
}

void NetClient::Warning(const QString &str)
{
	textEditSocket->append(str);
	MyQTextEdit::ColorizeLastCount(textEditSocket, Qt::blue, str.size());
}

void NetClient::CreateSocket()
{
	socket = new QTcpSocket(this);
	socket->connectToHost("127.0.0.1", port);
	connect(socket, &QTcpSocket::connected, this, &NetClient::SlotConnected);
	connect(socket, &QTcpSocket::readyRead, this, &NetClient::SlotReadyRead);
	connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(SlotError(QAbstractSocket::SocketError)));
	// в старом стиле из-за кривизны qt
}

void NetClient::CreateWindowSocket(bool show)
{
	widget = std::make_unique<QWidget>();
	widget->setWindowTitle("NetClient");

	QVBoxLayout *vlo_main = new QVBoxLayout(widget.get());
	QHBoxLayout *hlo1 = new QHBoxLayout;
	QHBoxLayout *hlo2 = new QHBoxLayout;
	vlo_main->addLayout(hlo1);
	vlo_main->addLayout(hlo2);

	QPushButton *btnTestSend = new QPushButton("test send");
	hlo1->addWidget(btnTestSend);
	connect(btnTestSend,&QPushButton::clicked,[this](){ SendToServer("test send", true); });

	QPushButton *btnTestRequest = new QPushButton("test");
	hlo1->addWidget(btnTestRequest);
	connect(btnTestRequest,&QPushButton::clicked, this, &NetClient::SlotTest);

	hlo1->addWidget(new QLabel("arg:"));
	leArg = new QLineEdit;
	hlo1->addWidget(leArg);

	hlo1->addStretch();

	textEditSocket = new QTextEdit;
	hlo2->addWidget(textEditSocket);

	if(show) widget->show();

	widget->resize(800,600);
	QTimer::singleShot(100,[this](){ widget->move(50, 50); widget->activateWindow(); });
}

void NetClient::SlotReadyRead()
{
	QTcpSocket *sock = (QTcpSocket*)sender();
	QString readed = sock->readAll();
	if(readed.endsWith(';')) readed.chop(1);
	else
	{
		Error("get unfinished data ["+readed+"]");
		return;
	}
	auto commands = readed.split(NetConstants::end_marker());

	for(auto &command:commands)
	{
		if(command == NetConstants::auth_success())
		{
			canNetwork = true;
			Log(NetConstants::auth_success());
		}
		else if(command == NetConstants::auth_failed())
		{
			canNetwork = false;
			Error(NetConstants::auth_failed());
		}
		else if(command.startsWith(NetConstants::last_update()))
		{
			QStringRef lastUpdate(&command, NetConstants::last_update().size(), command.size() - NetConstants::last_update().size());
			Log("last update on server is " + lastUpdate);
		}
		else if(command.startsWith(NetConstants::request_answ()))
		{
			RequestsAnswersWorker(std::move(command));
		}
		else if(command == NetConstants::unexpacted())
		{
			Error("server says that get unexpacted data from me");
		}
		else
		{
			Error("received unexpacted data from server {" + command + "}");
		}
	}
}

void NetClient::SlotError(QAbstractSocket::SocketError err)
{
	QString str;
	if(err == QAbstractSocket::HostNotFoundError) str += "Error: Host not foud. ";
	if(err == QAbstractSocket::RemoteHostClosedError) str += "Error: Remote host is closed. ";
	if(err == QAbstractSocket::ConnectionRefusedError) str += "Error: Connection was refused. ";
	str += socket->errorString();
	Error(str);
}

void NetClient::SendToServer(QString str, bool sendEndMarker)
{
	CodeMarkers::can_be_optimized("takes copy, than make other copy");

	str.replace(NetConstants::end_marker(), NetConstants::end_marker_replace());
	socket->write(str.toUtf8());
	if(sendEndMarker) socket->write(NetConstants::end_marker().toUtf8());
}

void NetClient::RequestToServer(const QString &requestType, QString content, AnswerWorkerFunction answWorker)
{
	CodeMarkers::can_be_optimized("takes copy, than make other copy");

	int idRqst = TakeIdRequest();
	requestAswerWorkers[idRqst] = std::move(answWorker);
	CodeMarkers::to_do("а у запроса должен быть еще и срок действия, который если вышел - то ГАЛЯ у нас отмена");

	content.replace(NetConstants::end_marker(), NetConstants::end_marker_replace());
	socket->write(NetConstants::request().toUtf8());
	socket->write(QSn(idRqst).append(' ').toUtf8());
	socket->write(requestType.toUtf8());
	socket->write(" ");
	socket->write(content.toUtf8()); // <
	socket->write(NetConstants::end_marker().toUtf8());
}

void NetClient::RequestsAnswersWorker(QString text)
{
	CodeMarkers::can_be_optimized("give copy text to DecodeRequestCommand, but can move, but it used in error");
	auto requestAnswerData = NetClient::DecodeRequestAnswer(text);
	if(!requestAnswerData.errors.isEmpty())
	{
		Error("error decoding request answer: "+requestAnswerData.errors + "; full text:\n"+text);
		SendToServer("error decoding request answer: "+requestAnswerData.errors, true);
		return;
	}

	auto it = requestAswerWorkers.find(requestAnswerData.id.toInt());
	if(it == requestAswerWorkers.end())
	{
		Error("error working request answer, not found worker for id; full text:\n"+text);
		SendToServer("error working request answer, not found worker for id; full text:\n"+text, true);
		return;
	}

	it->second(std::move(requestAnswerData));
}

NetClient::RequestData NetClient::DecodeRequestCommand(QString command)
{
	CodeMarkers::can_be_optimized("can be faster, no remove+LeftRef, but take MidRef");

	RequestData data;

	if(!command.startsWith(NetConstants::request())) { data.errors = "bad start"; return data; }

	command.remove(0,NetConstants::request().size());

	int spaceIndex = command.indexOf(' ');
	if(spaceIndex == -1) { data.errors = "not found space to close id"; return data; }

	if(IsInt(command.leftRef(spaceIndex))) data.id = command.leftRef(spaceIndex).toString();
	else { data.errors = "bad id"; return data; }

	command.remove(0,spaceIndex +1);

	spaceIndex = command.indexOf(' ');
	if(spaceIndex == -1) { data.errors = "not found space to close requestType"; return data; }
	data.type = command.left(spaceIndex);
	if(NetConstants::allReuestTypes().count(data.type) == 0) { data.errors = "bad requestType"; data.type = ""; return data; }

	command.remove(0,spaceIndex +1);

	data.content = std::move(command);

	return data;
}

NetClient::RequestData NetClient::DecodeRequestAnswer(QString command)
{
	RequestData data;

	if(!command.startsWith(NetConstants::request_answ())) { data.errors = "bad start"; return data; }

	command.remove(0,NetConstants::request_answ().size());

	int spaceIndex = command.indexOf(' ');
	if(spaceIndex == -1) { data.errors = "not found space to close id"; return data; }

	if(IsInt(command.leftRef(spaceIndex))) data.id = command.leftRef(spaceIndex).toString();
	else { data.errors = "bad id"; return data; }

	command.remove(0,spaceIndex +1);

	data.content = std::move(command);

	return data;
}

void NetClient::SlotConnected()
{
	Log("connected to server, auth try");

	QCryptographicHash hash(QCryptographicHash::Md5);
	hash.addData((NetConstants::test_passwd()).toUtf8());
	SendToServer(NetConstants::auth().toUtf8() + hash.result().toHex(), true);
}
