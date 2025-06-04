#ifndef NETCLIENT_H
#define NETCLIENT_H

#include <memory>
#include <queue>

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

struct Note;

class NetClient : public QObject
{
	Q_OBJECT
public:
	explicit NetClient(QObject *parent = nullptr) : QObject(parent)
	{
		CreateWindowSocket(true);
		CreateSocket();

		timerSynch = new QTimer(this);
		connect(timerSynch, &QTimer::timeout, [this](){ SynchFromQueue(); });
		timerSynch->start(100);
	}
	void SlotTest();

	void Log(const QString &str, bool appendLastRow = false);
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
	void SlotConnected();
	void SlotReadyRead();
	void SlotError(QAbstractSocket::SocketError err);

public: signals:
	void SignalNoteRemoved(qint64 idOnClient);
	void SignalNoteChangedGgroup(qint64 idOnClient);

public:
	void SendToServer(QString str, bool appendInLastRow);

	declare_struct_3_fields_move(MsgData, QString, type, QString, content, QString, errors);
	void MsgToServer(const QString &msgType, QString content);
	static MsgData DecodeMsg(QString msg);

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

	void SynchronizeNote(Note *note);
	std::queue<NetConstants::SynchData> synchQueue;
	QTimer *timerSynch = nullptr;
	void SynchFromQueue();

	declare_struct_3_fields_move(CommandData, QString, commandName, QString, content, QString, errors);
	void CommandsToClientWorker(QString text);
	static QString PrepareCommandToClient(const QString &commandName, const QString &content);
	static CommandData DecodeCommandToClient(QString command);
	void command_remove_note_worker(QString && commandContent);

	std::map<QStringRefWr_const, std::function<void(QString && commandContent)>> commandsWorkersMap {
		{ std::cref(NetConstants::command_remove_note()), [this](QString && commandContent){ command_remove_note_worker(std::move(commandContent)); } },
	};
};

#endif // NETCLIENT_H
