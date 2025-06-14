#ifndef NETCLIENT_H
#define NETCLIENT_H

#include <memory>
#include <queue>

#include <QObject>
#include <QLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QTimer>
#include <QCryptographicHash>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkAccessManager>

#include "declare_struct.h"

#include "NetConstants.h"

class ISocket
{
public:
	virtual ~ISocket() {}
};

class Requester
{
public:
	virtual ~Requester() {}

	virtual void Log(const QString &str, bool appendInLastRow = false) = 0;
	virtual void Error(const QString &str) = 0;
	virtual void Warning(const QString &str) = 0;

	virtual void Write(ISocket * sock, const QString &str) = 0;

	void SendInSock(ISocket *sock, QString str, bool sendEndMarker);

	declare_struct_4_fields_move(RequestData, QString, id, QString, type, QString, content, QString, errors);
	using AnswerWorkerFunction = std::function<void(QString &&answContent)>;
	void RequestInSock(ISocket *sock, const QString &requestType, QString content, AnswerWorkerFunction answWorker);

	void RequestsWorker(ISocket *sock, QString text);
	virtual std::map<QStringRefWr_const, std::function<void(ISocket *sock, RequestData &&requestData)>> & RequestWorkersMap() = 0;
	void AnswerForRequestSending(ISocket *sock, RequestData &requestData, QString str);
	uint answDelay = 0;

	void RequestsAnswersWorker(QString text);

	static RequestData DecodeRequestCommand(QString command);
	static RequestData DecodeRequestAnswer(QString command);
	int TakeIdRequest() { return idRequest++; };
	int idRequest = 1;
	std::map<int, AnswerWorkerFunction> requestAswerWorkers; // int = id
};

struct Note;

class NetClient : public QObject, public ISocket, public Requester
{
	Q_OBJECT
public:
	explicit NetClient(QObject *parent = nullptr);
	~NetClient();
	void SlotTest();
	void SlotTestSend();

	virtual void Log(const QString &str, bool appendInLastRow = false) override;
	virtual void Error(const QString &str) override;
	virtual void Warning(const QString &str) override;

	std::unique_ptr<QWidget> widget;
	QLineEdit *leArg;
	QTextEdit *textEditSocket;
	bool canNetwork = false;
	bool pollyWhaits = false;

	QNetworkAccessManager *manager {};
	QTimer *timerPolly {};
	QString adress;
	QString bufForTarget;

	void CreateSocket();
	void Create2Window(bool show);

	virtual void Write(ISocket * /*sock*/, const QString &str) override;

public slots:
	void SlotReadyRead(QNetworkReply *reply);

public: signals:
	void SignalNoteRemoved(qint64 idOnClient);
	void SignalNoteChangedGgroup(qint64 idOnClient);
	void SignalNoteUpdated(qint64 idOnClient);
	void SignalNewNoteAppeared(qint64 idOnClient);

public:
	declare_struct_3_fields_move(MsgData, QString, type, QString, content, QString, errors);
	void MsgToServer(const QString &msgType, QString content);
	static MsgData DecodeMsg(QString msg);

	void RequestToServerWithWait(const QString &requestType, QString content, AnswerWorkerFunction answWorker);

	void SynchronizeAllNotes(std::vector<Note*> allClientNotes);
private:
	std::queue<NetConstants::SynchData> synchQueue;
	static const QString& EndAllNotesMarker() { static QString str = "EndAllNotesMarker"; return str; }
	QTimer *timerSynch = nullptr;
	void SynchFromQueue();

public:
	declare_struct_3_fields_move(CommandData, QString, commandName, QString, content, QString, errors);
	void CommandsToClientWorker(QString text);
	static QString PrepareCommandToClient(const QString &commandName, const QString &content);
	static CommandData DecodeCommandToClient(QString command);
	void command_remove_note_worker(QString && commandContent);
	void command_update_note_worker(QString && commandContent);

	std::map<QStringRefWr_const, std::function<void(QString && commandContent)>> commandsWorkersMap {
		{ std::cref(NetConstants::command_remove_note()), [this](QString && commandContent){ command_remove_note_worker(std::move(commandContent)); } },
		{ std::cref(NetConstants::command_update_note()), [this](QString && commandContent){ command_update_note_worker(std::move(commandContent)); } },
	};

	void request_get_note_worker(ISocket *sock, NetClient::RequestData && requestData);

	std::map<QStringRefWr_const, std::function<void(ISocket *sock, NetClient::RequestData &&requestData)>> requestWorkersMap {
		{ std::cref(NetConstants::request_get_note()), [this](ISocket *sock, NetClient::RequestData &&requestData){ request_get_note_worker(sock, std::move(requestData)); } },
	};
	virtual std::map<QStringRefWr_const, std::function<void(ISocket *sock, RequestData &&requestData)>> & RequestWorkersMap() override
	{ return requestWorkersMap; }

	QString& PrepareTargetWithAuth();
	static bool ChekAuth(const QString &targetOnServerSide);
};

#endif // NETCLIENT_H
