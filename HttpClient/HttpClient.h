#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include <QDebug>
#include <QString>

#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <thread>
#include <atomic>
#include <functional>

class HttpClient  {
public:
	HttpClient();
	~HttpClient();

	std::function<void(const QString&)> logFoo;
	void Log(const QString &str);
	void Error(const QString &str);

	template<class T>
	static QString GetFullText(T &container)
	{
		std::stringstream streamForOutput;
		streamForOutput << container;
		return "["+QString::fromStdString(streamForOutput.str())+"]";
	}

	void start(const std::string& host, unsigned short port, const std::string& target);
	void stop();
	bool isRunning() const;

private:
	void run(const std::string& host, unsigned short port, const std::string& target);

	std::thread clientThread;
	std::atomic<bool> running{false};
	QObject invoker;
};

#endif // HTTPCLIENT_H
