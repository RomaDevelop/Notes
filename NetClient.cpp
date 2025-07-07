#include "NetClient.h"

#include <QProgressDialog>
#include <QCoreApplication>
#include <QMessageBox>
#include <QPointer>
#include <QLabel>
#include <QComboBox>
#include <QNetworkProxy>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QNetworkReply>
#include <QMessageAuthenticationCode>

#include "InputBlocker.h"

#include "MyQShortings.h"
#include "MyQTextEdit.h"
#include "MyCppDifferent.h"
#include "MyCppRandom.h"
#include "MyQDialogs.h"
#include "CodeMarkers.h"
#include "TextEditCleaner.h"

#include "DataBase.h"
#include "Note.h"

NetClient::NetClient(QObject *parent) : QObject(parent)
{
	CreateWidgets(true);

	//	QNetworkProxy *proxyPtr = new QNetworkProxy;
	//	QNetworkProxy &proxy = *proxyPtr;
	//	proxy.setType(QNetworkProxy::HttpProxy); // Тип прокси (HTTP, SOCKS и т.д.)
	//	proxy.setHostName("10.123.2.222"); // Адрес прокси-сервера
	//	proxy.setPort(3128); // Порт прокси-сервера
	//	proxy.setUser("MyslivchenkoRI"); // Имя пользователя (если требуется)
	//	proxy.setPassword("GenCtr2025"); // Пароль (если требуется)

	//	// Устанавливаем прокси для QNetworkAccessManager
	//	QNetworkProxy::setApplicationProxy(proxy);
	//QNetworkProxyFactory::setUseSystemConfiguration(true);

	CreateSocket();

	timerSynch = new QTimer(this);
	connect(timerSynch, &QTimer::timeout, [this](){ SynchFromQueue(); });
	timerSynch->start(100);
}

NetClient::~NetClient()
{

}

void NetClient::SlotTest()
{
//	QNetworkAccessManager *manager = new QNetworkAccessManager();

//	QUrl qurl("http://localhost:25001");
//	QNetworkRequest request(qurl);
//	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

//	// Отправка POST-запроса
//	QNetworkReply *reply = manager->post(request, "1 ывывы END");

//	// Обработка ответа
//	QObject::connect(reply, &QNetworkReply::finished, [reply]() {
//		if (reply->error() == QNetworkReply::NoError) {
//			qDebug() << "Response:" << reply->readAll();
//		} else {
//			qDebug() << "Error:" << reply->errorString();
//		}
//		reply->deleteLater();
//	});
}

void NetClient::SlotTestSend()
{
	QString s = "";
	for(int i=0; i<25; i++)
		s += 'a' + MyCppRandom::Get(0,25);
	SendInSock(this, "test send " + s, true);
}

void NetClient::Log(const QString &str, bool appendInLastRow)
{
	if(appendInLastRow) MyQTextEdit::AppendInLastRow(textEditSocket, str);
	else textEditSocket->append(str);
	MyQTextEdit::ColorizeLastCount(textEditSocket, Qt::black, str.size());
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

void NetClient::InitPollyCloserTimer()
{
	if(timerPolly) timerPolly->deleteLater();
	timerPolly = new QTimer(this);
	pollyWhaits = -1;
	connect(timerPolly, &QTimer::timeout, [this](){
		if(sessionId <= 0) { pollyWhaits = -1; return; }

		if(pollyWhaits == -1 || pollyWhaits >= NetConstants::pollyMaxWaitClientMs)
		{
			auto pollyAnsw = [this](QString &&answContent){
				pollyWhaits = -1;
				Log("pollyAnsw get " + answContent);
				WorkReaded(answContent);
			};
			RequestInSock(this, NetConstants::request_polly(), NetConstants::nothing(), pollyAnsw);
			pollyWhaits = 0;
		}
		else pollyWhaits += pollyTimerTimeoutMs;
	});
	timerPolly->start(pollyTimerTimeoutMs);
}

void NetClient::CreateSocket()
{
	if(timerPolly) { timerPolly->deleteLater(); timerPolly = {}; }
	if(manager) manager->deleteLater();
	manager = new QNetworkAccessManager(this);
#ifdef QT_DEBUG
	adress = "http://127.0.0.1:25002";
#else
	adress = "http://83.217.213.213:25002";
#endif
	connect(manager, &QNetworkAccessManager::finished, this, &NetClient::SlotReadyRead);

	RequestGetSessionId();

	InitPollyCloserTimer();
}



void NetClient::CreateWidgets(bool show)
{
	widget = std::make_unique<QWidget>();
	widget->setWindowTitle("NetClient");

	QVBoxLayout *vlo_main = new QVBoxLayout(widget.get());
	QHBoxLayout *hlo1 = new QHBoxLayout;
	QHBoxLayout *hlo2 = new QHBoxLayout;
	QHBoxLayout *hlo3 = new QHBoxLayout;
	vlo_main->addLayout(hlo1);
	vlo_main->addLayout(hlo2);
	vlo_main->addLayout(hlo3);

	QPushButton *btnTestClear = new QPushButton("clear");
	hlo1->addWidget(btnTestClear);
	connect(btnTestClear,&QPushButton::clicked,[this](){ textEditSocket->clear(); });

	QPushButton *btnConnect = new QPushButton("connect");
	hlo1->addWidget(btnConnect);
	connect(btnConnect,&QPushButton::clicked,[this](){ CreateSocket(); });

	QPushButton *btnDisconnect = new QPushButton("disconnect");
	hlo1->addWidget(btnDisconnect);
	connect(btnDisconnect,&QPushButton::clicked,[this](){ manager->deleteLater(); manager = {}; });

	QPushButton *btnSqlClearNotes = new QPushButton(" clear notes ");
	hlo1->addWidget(btnSqlClearNotes);
	connect(btnSqlClearNotes,&QPushButton::clicked,[](){
		QMbError("disabled");
		//DataBase::DoSqlQuery("delete from " + Fields::Notes());
	});

	QPushButton *btnTestSend = new QPushButton("test send");
	hlo1->addWidget(btnTestSend);
	connect(btnTestSend,&QPushButton::clicked,  this, &NetClient::SlotTestSend);

	QPushButton *btnTestRequest = new QPushButton("test");
	hlo1->addWidget(btnTestRequest);
	connect(btnTestRequest,&QPushButton::clicked, this, &NetClient::SlotTest);

	hlo1->addWidget(new QLabel("arg:"));
	leArg = new QLineEdit;
	hlo1->addWidget(leArg);

	hlo1->addStretch();

	textEditSocket = new QTextEdit;
	hlo3->addWidget(textEditSocket);

	new TextEditCleaner(textEditSocket, 1000, textEditSocket);

	if(show) widget->show();

	widget->resize(650,600);
	QTimer::singleShot(100,[this](){ widget->move(700, 10); widget->activateWindow(); });
}

void NetClient::Write(ISocket *, const QString &str)
{
	QNetworkRequest request(PrepareTarget());
	request.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
	manager->post(request, str.toUtf8());
}

void NetClient::SlotReadyRead(QNetworkReply *reply)
{
	QString readed;
	if (reply->error()) {
		Error(reply->errorString());
	}
	else {
		readed = reply->readAll();
	}
	reply->deleteLater();

	if(readed.endsWith(';')) readed.chop(1);
	else
	{
		Error("get unfinished data ["+readed+"]");
		return;
	}

	WorkReaded(readed);
}

void NetClient::WorkReaded(const QString &readed)
{
	auto msgs = readed.split(NetConstants::end_marker());

	for(auto &msg:msgs)
	{
		msg.replace(NetConstants::end_marker_replace(), NetConstants::end_marker());
		Log("received message: " + msg);

		if(msg == NetConstants::auth_failed())
		{
			sessionId = 0;
			Error(NetConstants::auth_failed());
		}
		else if(msg.startsWith(NetConstants::request()))
		{
			RequestsWorker(this, std::move(msg));
		}
		else if(msg.startsWith(NetConstants::request_answ()))
		{
			RequestsAnswersWorker(std::move(msg));
		}
		else if(msg.startsWith(NetConstants::command_to_client()))
		{
			CommandsToClientWorker(std::move(msg));
		}
		else if(msg == NetConstants::nothing())
		{
			Log("received message {"+NetConstants::nothing()+"} from server");
		}
		else if(msg == NetConstants::unexpected())
		{
			Error("server says that get unexpected data from me");
		}
		else
		{
			Error("received unexpacted data from server {" + msg + "}");
		}
	}
}

void NetClient::MsgToServer(const QString &msgType, QString content)
{
	content.prepend(" ").prepend(msgType).prepend(NetConstants::msg());
	SendInSock(this, std::move(content), true);
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

void NetClient::RequestToServerWithWait(const QString & requestType, QString content, Requester::AnswerWorkerFunction answWorker)
{
	if(sessionId <= 0)
	{
		QMbError("Server not connected, operation impossible");
		return;
	}

	if(!answWorker) Warning("RequestToServerWithWait called without answWorker");

	static auto blocker = new InputBlocker(qApp);
	qApp->installEventFilter(blocker);

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
			qApp->removeEventFilter(blocker);
			QMbError("Server doesn't response");
		}
		progress->hide();
		progress->deleteLater();
	});

	NetClient::AnswerWorkerFunction answFoo = [this, progressQPtr, answWorkerMoved = std::move(answWorker)](QString &&answContent){
		Log("answ get: " + answContent);
		if(progressQPtr)
		{
			qApp->removeEventFilter(blocker);
			progressQPtr->cancel();
			progressQPtr->close();
			if(answWorkerMoved) answWorkerMoved(std::move(answContent));
		}
		else Error("but it is too late to work it");
	};

	RequestInSock(this, requestType, content, std::move(answFoo));
}

void NetClient::SynchronizeAllNotes(std::vector<Note*> allClientNotes)
{
	if(0) CodeMarkers:: to_do("склейку запросов на синхронизацию для одной заметки");
	for(auto &note:allClientNotes)
	{
		if(DataBase::IsGroupLocalByName(note->group)) continue;
		synchQueue.emplace(NetConstants::SynchData(QSn(note->idOnServer), note->DtLastUpdatedStr()));
	}
	if(!synchQueue.empty())
		synchQueue.emplace(NetConstants::SynchData(EndAllNotesMarker(), {}));
	else Log("SynchronizeAllNotes called. Nothing to synch");
}

void NetClient::SynchFromQueue()
{
	if(sessionId <= 0) return;
	if(synchQueue.empty()) return;

	bool endAllNotesMarkerFound = false;
	std::vector<NetConstants::SynchData> datas;
	for(int i=0; i<10 && !synchQueue.empty(); i++)
	{
		if(synchQueue.front().idOnServer == EndAllNotesMarker())
		{
			endAllNotesMarkerFound = true;
		}
		else
		{
			datas.push_back(std::move(synchQueue.front()));
		}

		synchQueue.pop();
	}
	QString request = NetConstants::MakeRequest_synch_note(datas);
	if(request.isEmpty()) { Error("SynchFromQueue result MakeRequest_synch_note is empty"); return; }

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
				Log("получен ответ об успшной обработке запроса на синхронизацию заметки " + datas_[i].idOnServer);
			else if(results[i] == NetConstants::request_synch_res_error())
				Error("geted bad result for synch "+datas_[i].idOnServer);
			else
			{
				Error("synch result unknown value: "+results[i]);
				MsgToServer(NetConstants::msg_error(), "synch result unknown value: "+results[i]);
			}
		}
	};

	RequestInSock(this, NetConstants::request_synch_note(), request, std::move(answFoo));

	if(endAllNotesMarkerFound)
	{
		MsgToServer(NetConstants::msg_all_client_notes_synch_sended(), {});

		MsgToServer(NetConstants::msg_highest_idOnServer(), DataBase::HighestIdOnServer());
	}
}

void NetClient::RequestGetSessionId()
{
	auto answ = [this](QString &&answContent){
		sessionId = answContent.toULongLong();

		if(sessionId <= 0) Error("Get bad session id: " + QSn(sessionId));
		else Log("Get session id: " + QSn(sessionId));
	};

	sessionId = 0;
	RequestInSock(this, NetConstants::request_get_session_id(), NetConstants::nothing(), answ);
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
	if(!IsUInt(idOnServer))
	{
		Error("command_remove_note_worker get bad idOnServer: " + idOnServer);
		MsgToServer(NetConstants::msg_error(), "command_remove_note_worker get bad idOnServer: " + idOnServer);
		return;
	}
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
			if(DataBase::MoveNoteToGroupOnClient(QSn(note.id), DataBase::DefaultGroupId2(),
												 QDateTime::currentDateTime().toString(Fields::dtFormatLastUpated())))
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

void NetClient::command_update_note_worker(QString && commandContent)
{
	auto note = Note::LoadNote(commandContent);
	if(!note) {
		Error("command_update_note_worker: cant load note from data: " + commandContent);
		MsgToServer(NetConstants::msg_error(), "command_update_note_worker: cant load note from data: " + commandContent);
		return;
	}

	auto rec = DataBase::NoteByIdOnServer(QSn(note->idOnServer));
	if(rec.isEmpty())
	{
		note->id = DataBase::InsertNoteInClientDB(note.get());
		emit SignalNewNoteAppeared(note->id);
		return;
	}

	note->id = rec[Fields::idNoteInd].toLongLong();

	if(note->id <= 0)
	{
		Error("can't define local note id " + note->Name());
		return;
	}

	auto res = DataBase::SaveNoteOnClient(note.get());
	if(res.isEmpty()) emit SignalNoteUpdated(note->id);
	else
	{
		Error("command_update_note_worker: saving note error: " + res);
		MsgToServer(NetConstants::msg_error(), "command_update_note_worker: saving note error: " + res);
	}
}

void NetClient::request_get_note_worker(ISocket * sock, NetClient::RequestData && requestData)
{
	QString &idOnServer = requestData.content;
	auto rec = DataBase::NoteByIdOnServer(idOnServer);
	if(rec.isEmpty())
	{
		Error("CheckNoteId "+idOnServer+" false");
		AnswerForRequestSending(sock, std::move(requestData), NetConstants::invalid());
		return;
	}

	Note note;
	note.InitFromRecord(rec);
	AnswerForRequestSending(sock, std::move(requestData), note.ToStr_v1());
}

QString &NetClient::PrepareTarget()
{
	QString dtStr = QDateTime::currentDateTime()/*.addMSecs(-14980)*/.toString(NetConstants::auth_date_time_format());
												// 14980 - проходит, 14990 не проходит

	QByteArray hmac = QMessageAuthenticationCode::hash(dtStr.toUtf8(), NetConstants::test_passwd().toUtf8(), QCryptographicHash::Sha256);
	
	bufForTarget = adress;
	bufForTarget.append("/").append(dtStr).append("&").append(hmac.toHex());
	bufForTarget.append("&").append(QSn(sessionId));
	
	return bufForTarget;
}

NetClient::TargetContent NetClient::DecodeTarget(const QString &targetOnServerSide)
{
	QStringRef payload = targetOnServerSide.midRef(1);
	auto parts = payload.split('&');
	if (parts.size() < 3) return {};

	return TargetContent(parts[0], parts[1], parts[2]);
}

bool NetClient::ChekAuth(const TargetContent &targetContent)
{
	if(QDateTime::fromString(targetContent.dtStrRef.toString(), NetConstants::auth_date_time_format()).msecsTo(QDateTime::currentDateTime())
			> 15000)
		return false;

	QByteArray hmacEtalon = QMessageAuthenticationCode::hash(targetContent.dtStrRef.toUtf8(),
															 NetConstants::test_passwd().toUtf8(),
															 QCryptographicHash::Sha256);

	return hmacEtalon.toHex() == targetContent.hmac;
}

void Requester::SendInSock(ISocket * sock, QString str, bool sendEndMarker)
{
	CodeMarkers::can_be_optimized("takes copy, than make other copy");
	
	Log("SendInSock: sending: " + (str == " " ? "_space_" : sendEndMarker ? str + NetConstants::end_marker() : str));
	
	PrepareStringToSend(str, sendEndMarker);

	Write(sock, str);
}

void Requester::PrepareStringToSend(QString &str, bool addEndMarker)
{
	str.replace(NetConstants::end_marker(), NetConstants::end_marker_replace());
	if(addEndMarker) str.append(NetConstants::end_marker());
}

void Requester::RequestInSock(ISocket *sock, const QString & requestType, QString content, Requester::AnswerWorkerFunction answWorker)
{
	CodeMarkers::can_be_optimized("takes copy, than make other copy");
	
	CodeMarkers::to_do("нужно удалять из requestAswerWorkers спустя время если не пришло");

	if(content.isEmpty())
	{
		Warning("Requester::RequestInSock called with empty content. If content is not required should set content "
				"NetConstants::nothing()");
		content = NetConstants::nothing();
	}

	int idRqst = TakeIdRequest();
	requestAswerWorkers[idRqst] = std::move(answWorker);
	CodeMarkers::to_do("а у запроса должен быть еще и срок действия, который если вышел - то ГАЛЯ у нас отмена");

	QString tmp = NetConstants::request();
	tmp.append(QSn(idRqst)).append(' ').append(requestType).append(' ');
	content.prepend(tmp);
	SendInSock(sock, content, true);
}

void Requester::RequestsWorker(ISocket * sock, QString text)
{
	CodeMarkers::can_be_optimized("give copy text to DecodeRequestCommand, but can move, but it used in error");
	auto requestData = NetClient::DecodeRequestCommand(text);
	if(!requestData.errors.isEmpty())
	{
		Error("error decoding request: "+requestData.errors + "; full text:\n"+text);
		AnswerForRequestSending(sock, std::move(requestData), NetConstants::not_did());
		// почему стоит ответить - чтобы клиент не ждал пока истечет таймаут
		return;
	}

	Log("get reuqest "+requestData.type+" "+requestData.id+", start work");

	if(auto it = RequestWorkersMap().find(requestData.type); it != RequestWorkersMap().end())
	{
		std::function<void(ISocket *sock, NetClient::RequestData &&requestData)> &requestWorker = it->second;
		requestWorker(sock, std::move(requestData));
	}
	else
	{
		Error("Unrealesed requestData.type " + requestData.type);
		AnswerForRequestSending(sock, std::move(requestData), NetConstants::not_did());
		// почему стоит ответить - чтобы клиент не ждал пока истечет таймаут
	}
}

void Requester::AnswerForRequestSending(ISocket * sock, RequestData && requestData, QString str)
{
	MyCppDifferent::sleep_ms(answDelay);

	if(requestData.id.isEmpty()) {
		Error("AnswerForRequestSending executed with empty requestData.id=["+requestData.id+"]");
		return;
	}

	if(requestData.content.isEmpty())
		Warning("AnswerForRequestSending executed with empty requestData.content");

	str.prepend(requestData.id.append(" "));
	str.prepend(NetConstants::request_answ());
	SendInSock(sock, std::move(str), true);
}

void Requester::RequestsAnswersWorker(QString text)
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

Requester::RequestData Requester::DecodeRequestCommand(QString command)
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
	if(NetConstants::allReuestTypes().count(data.type) == 0)
		{ data.errors = "requestType not found in map NetConstants::allReuestTypes()"; data.type = ""; return data; }

	command.remove(0,spaceIndex +1);

	data.content = std::move(command);

	return data;
}

Requester::RequestData Requester::DecodeRequestAnswer(QString command)
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
