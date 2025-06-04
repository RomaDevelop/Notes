#include "NetClient.h"

#include <QProgressDialog>
#include <QCoreApplication>
#include <QMessageBox>
#include <QPointer>
#include <QLabel>

#include "InputBlocker.h"

#include "MyQShortings.h"
#include "MyQTextEdit.h"
#include "MyCppDifferent.h"
#include "MyQDialogs.h"
#include "CodeMarkers.h"

#include "DataBase.h"
#include "Note.h"

void NetClient::SlotTest()
{
	Log(DataBase::DoSqlQueryGetFirstRec("select idgroup from Notes where idNote=60").join(" "));
}

void NetClient::Log(const QString &str, bool appendInLastRow)
{
	if(appendInLastRow) MyQTextEdit::AppendInLastRow(textEditSocket, str);
	else textEditSocket->append(str);
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
	socket->deleteLater();
	socket = new QTcpSocket(this);
	socket->connectToHost("127.0.0.1", port);
	connect(socket, &QTcpSocket::connected, this, &NetClient::SlotConnected);
	connect(socket, &QTcpSocket::disconnected, [this](){ canNetwork = false; });
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

	QPushButton *btnTestClear = new QPushButton("clear");
	hlo1->addWidget(btnTestClear);
	connect(btnTestClear,&QPushButton::clicked,[this](){ textEditSocket->clear(); });

	QPushButton *btnTestConnect = new QPushButton("connect");
	hlo1->addWidget(btnTestConnect);
	connect(btnTestConnect,&QPushButton::clicked,[this](){ CreateSocket(); });

	QPushButton *btnTestDisconnect = new QPushButton("disconnect");
	hlo1->addWidget(btnTestDisconnect);
	connect(btnTestDisconnect,&QPushButton::clicked,[this](){ if(socket) socket->disconnect(); });

	QPushButton *btnSqlClearNotes = new QPushButton(" clear notes ");
	hlo1->addWidget(btnSqlClearNotes);
	connect(btnSqlClearNotes,&QPushButton::clicked,[](){
		DataBase::DoSqlQuery("delete from " + Fields::Notes());
	});

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

	widget->resize(650,600);
	QTimer::singleShot(100,[this](){ widget->move(700, 10); widget->activateWindow(); });
}

void NetClient::SlotConnected()
{
	Log("connected to server, auth try");

	QCryptographicHash hash(QCryptographicHash::Md5);
	hash.addData((NetConstants::test_passwd()).toUtf8());
	SendToServer(NetConstants::auth().toUtf8() + hash.result().toHex(), true);
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
		command.replace(NetConstants::end_marker_replace(), NetConstants::end_marker());
		Log("received command: " + command);

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
		else if(command.startsWith(NetConstants::command_to_client()))
		{
			CommandsToClientWorker(std::move(command));
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

	Log("sending: " + (str == " " ? "_space_" : sendEndMarker ? str + NetConstants::end_marker() : str));

	str.replace(NetConstants::end_marker(), NetConstants::end_marker_replace());
	socket->write(str.toUtf8());
	if(sendEndMarker) socket->write(NetConstants::end_marker().toUtf8());

	Log(" | sended", true);
}

void NetClient::MsgToServer(const QString &msgType, QString content)
{
	SendToServer(NetConstants::msg(), false);
	SendToServer(msgType, false);
	SendToServer(" ", false);
	SendToServer(std::move(content), true);
}

NetClient::MsgData NetClient::DecodeMsg(QString msg)
{
	CodeMarkers::can_be_optimized("can be faster, no remove+LeftRef, but take MidRef");

	MsgData data;

	if(!msg.startsWith(NetConstants::msg())) { data.errors = "bad start msg: " + msg; return data; }

	msg.remove(0,NetConstants::msg().size());

	int spaceIndex = msg.indexOf(' ');
	if(spaceIndex == -1) { data.errors = "not found space to close msgType"; return data; }
	data.type = msg.left(spaceIndex);
	if(NetConstants::allMsgsTypes().count(data.type) == 0) { data.errors = "bad msgType"; data.type = ""; return data; }

	msg.remove(0,spaceIndex +1);

	data.content = std::move(msg);

	return data;
}

void NetClient::RequestToServer(const QString &requestType, QString content, AnswerWorkerFunction answWorker)
{
	CodeMarkers::can_be_optimized("takes copy, than make other copy");

	CodeMarkers::to_do("нужно удалять из requestAswerWorkers спустя время если не пришло");

	int idRqst = TakeIdRequest();
	requestAswerWorkers[idRqst] = std::move(answWorker);
	CodeMarkers::to_do("а у запроса должен быть еще и срок действия, который если вышел - то ГАЛЯ у нас отмена");

	SendToServer(NetConstants::request(), false);
	SendToServer(QSn(idRqst).append(' '), false);
	SendToServer(requestType, false);
	SendToServer(" ", false);
	SendToServer(content, true);
}

void NetClient::RequestToServerWithWait(const QString &requestType, QString content, AnswerWorkerFunction answWorker)
{
	if(!answWorker) Warning("RequestToServerWithWait called without answWorker");

	//static auto blocker = new InputBlocker(qApp);
	//qApp->installEventFilter(blocker);

	QProgressDialog *progress = new QProgressDialog("Resrver response waiting", "", 0, 0);
	QPointer<QProgressDialog> progressQPtr(progress);
	progress->setWindowModality(Qt::ApplicationModal);
	progress->setCancelButton(nullptr);

	if(0) CodeMarkers::to_do("make timer singleShot pool");
	QTimer::singleShot(500, [progress]() { if(!progress->wasCanceled()) progress->show(); });
	QTimer::singleShot(3000, [progress]()
	{
		if(!progress->wasCanceled())
		{
			//qApp->removeEventFilter(blocker);
			QMbError("Server doesn't response");
		}
		progress->hide();
		progress->deleteLater();
	});

	NetClient::AnswerWorkerFunction answFoo = [this, progressQPtr, answWorkerMoved = std::move(answWorker)](QString &&answContent){
		Log("answ get: " + answContent);
		if(progressQPtr)
		{
			//qApp->removeEventFilter(blocker);
			progressQPtr->cancel();
			progressQPtr->close();
			if(answWorkerMoved) answWorkerMoved(std::move(answContent));
		}
		else Error("but it is too late to work it");
	};

	RequestToServer(requestType, content, std::move(answFoo));
}

void NetClient::RequestsAnswersWorker(QString text)
{
	CodeMarkers::can_be_optimized("give copy text to DecodeRequestCommand, but can move, but it used in error");
	auto requestAnswerData = NetClient::DecodeRequestAnswer(text);
	if(!requestAnswerData.errors.isEmpty())
	{
		Error("error decoding request answer: "+requestAnswerData.errors + "; full text:\n"+text);
		return;
	}

	auto it = requestAswerWorkers.find(requestAnswerData.id.toInt());
	if(it == requestAswerWorkers.end())
	{
		Error("error working request answer, not found worker for id; full text:\n"+text);
		return;
	}

	it->second(std::move(requestAnswerData.content));

	requestAswerWorkers.erase(it);
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

void NetClient::SynchronizeNote(Note * note)
{
	if(0) CodeMarkers:: to_do("склейку запросов на синхронизацию для одной заметки");
	synchQueue.emplace(NetConstants::SynchData(note, QSn(note->idOnServer), note->DtLastUpdatedStr()));
}

void NetClient::SynchFromQueue()
{
	if(!canNetwork) return;
	if(synchQueue.empty()) return;

	std::vector<NetConstants::SynchData> datas;
	for(int i=0; i<10 && !synchQueue.empty(); i++)
	{
		datas.push_back(synchQueue.front());
		synchQueue.pop();
	}
	QString request = NetConstants::MakeRequest_synch_note(datas);

	AnswerWorkerFunction answFoo = [this, datas_ = std::move(datas)](QString &&answContent){
		if(0) CodeMarkers::to_do("опасность, тут может быть обращение к уже уничтоженной заметке");
		if(answContent == NetConstants::not_did())
		{
			Error("geted result not_did for synch");
			return;
		}

		auto results = answContent.split(',');
		if(results.size() != (int)datas_.size())
		{
			Error("synch result geted bad size: "+QSn(results.size())+"; in request were "+QSn(datas_.size()));
			MsgToServer(NetConstants::msg_error(), "synch result geted bad size: "+QSn(results.size())+"; in request were "+QSn(datas_.size()));
			return;
		}

		for(int i=0; i<results.size(); i++)
		{
			if(results[i] == NetConstants::request_synch_res_success())
				Log("получен ответ об успшной обработке запроса на синхронизацию заметки " + datas_[i].idOnSever);
			else if(results[i] == NetConstants::request_synch_res_error())
				Error("geted bad result for synch "+datas_[i].idOnSever);
			else
			{
				Error("synch result unknown value: "+results[i]);
				MsgToServer(NetConstants::msg_error(), "synch result unknown value: "+results[i]);
			}
		}
	};

	RequestToServer(NetConstants::request_synch_note(), request, std::move(answFoo));
}

void NetClient::CommandsToClientWorker(QString text)
{
	CodeMarkers::can_be_optimized("give copy text, but can move, but it used in error");
	auto commandData = NetClient::DecodeCommandToClient(text);
	if(!commandData.errors.isEmpty())
	{
		Error("error decoding command: "+commandData.errors + "; full text:\n"+text);
		return;
	}

	auto it = commandsWorkersMap.find(commandData.commandName);
	if(it == commandsWorkersMap.end())
	{
		Error("error working command, not found worker for id; full text:\n"+text);
		return;
	}

	it->second(std::move(commandData.content));
}

QString NetClient::PrepareCommandToClient(const QString & commandName, const QString & content)
{
	QString command;
	command.reserve(NetConstants::command_to_client().size() + commandName.size() + 1 + content.size());
	command.append(NetConstants::command_to_client()).append(commandName).append(' ').append(content);
	return command;
}

NetClient::CommandData NetClient::DecodeCommandToClient(QString command)
{
	CommandData data;

	if(!command.startsWith(NetConstants::command_to_client())) { data.errors = "bad start"; return data; }

	command.remove(0,NetConstants::command_to_client().size());

	int spaceIndex = command.indexOf(' ');
	if(spaceIndex == -1) { data.errors = "not found space to close commandName"; return data; }

	data.commandName = command.left(spaceIndex);

	command.remove(0,spaceIndex +1);

	data.content = std::move(command);

	return data;
}

void NetClient::command_remove_note_worker(QString && commandContent)
{
	/// на клиенте пока нет корзины - выводить сообщение, какие заметки предлагается удалить
	QString &idOnServer = commandContent;
	auto record = DataBase::NoteByIdOnServer(idOnServer);
	if(record.isEmpty())
	{
		CodeMarkers::to_do("если пришло сообщение удалить заметку, а она уже была удалена,"
						   "все норм, но если пришло говно - нужно сигнализировать. "
						   "но тогда нужно хранить ранее удалённые заметки");
	}
	else
	{
		Note note;
		note.InitFromRecord(record);
		auto answ = MyQDialogs::CustomDialog("Command from server", "Get command from server to remove note " + note.Name()
											 + "\n\nWhat should to do?",
											 {"Remove", "Move to default group"});
		if(0) CodeMarkers::to_do("can add action show note, but need make cykle choose actoion cdialog");
		if(answ == "Remove")
		{
			if(DataBase::RemoveNoteOnClient(QSn(note.id), true))
			{
				emit SignalNoteRemoved(note.id);
			}
			else
			{
				QMbError("This doesn't works now, check updates");
				MsgToServer(NetConstants::msg_error(), "client can't remove note");
			}
		}
		else if(answ == "Move to default group")
		{
			if(DataBase::MoveNoteToGroupOnClient(QSn(note.id), DataBase::DefaultGroupId(), QDateTime::currentDateTime().toString(Fields::dtFormatLastUpated())))
			{
				emit SignalNoteChangedGgroup(note.id);
			}
			else
			{
				QMbError("This doesn't works now, check updates");
				MsgToServer(NetConstants::msg_error(), "client can't move note to default group");
			}
		}
		else
		{
			QMbError("This doesn't works now, check updates");
			MsgToServer(NetConstants::msg_error(), "wrong answ in removindg action");
		}
	}
}


