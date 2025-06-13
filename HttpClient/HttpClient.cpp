#include "HttpClient.h"

#include <iostream>

#include "MyQShortings.h"

using tcp = boost::asio::ip::tcp;
namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

HttpClient::HttpClient() {}
HttpClient::~HttpClient() { stop(); }

void HttpClient::Log(const QString &str) { if(logFoo) logFoo(str); else qdbg << str; }

void HttpClient::Error(const QString &str) { if(logFoo) logFoo(str); else qdbg << str; }

void HttpClient::start(const std::string& host, unsigned short port, const std::string& target) {
	if (running) return;
	running = true;
	clientThread = std::thread([=] { run(host, port, target); });
}

void HttpClient::stop() {
	running = false;
	if (clientThread.joinable()) {
		clientThread.join();
	}
}

bool HttpClient::isRunning() const {
	return running;
}

void HttpClient::run(const std::string& host, unsigned short port, const std::string& target) {
	Log("HttpClient::run");
	try {
		net::io_context ioc;
		tcp::resolver resolver(ioc);
		beast::tcp_stream stream(ioc);

		auto const results = resolver.resolve(host, std::to_string(port));
		stream.connect(results);

		{
			// Формируем GET-запрос
			http::request<http::string_body> request{http::verb::get, target, 11};
			request.set(http::field::host, host);
			request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

			// Отправляем запрос
			http::write(stream, request);

			// Читаем ответ
			beast::flat_buffer buffer;
			http::response<http::string_body> response;
			http::read(stream, buffer, response);

			Log(QString("=== SENT REQUEST ===\n") + GetFullText(request));
			Log(QString("=== RESPONSE ===\n") + GetFullText(response));
			Log(QString("=== DECODED RESPONSE ===: [") + QString::fromStdString(response.body())+"]");
		}

		{
			// Формируем GET-запрос
			http::request<http::string_body> request{http::verb::get, target, 11};
			request.set(http::field::host, host);
			request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

			// Отправляем запрос
			http::write(stream, request);

			// Читаем ответ
			beast::flat_buffer buffer;
			http::response<http::string_body> response;
			http::read(stream, buffer, response);

			Log(QString("=== SENT REQUEST ===\n") + GetFullText(request));
			Log(QString("=== RESPONSE ===\n") + GetFullText(response));
			Log(QString("=== DECODED RESPONSE ===: [") + QString::fromStdString(response.body())+"]");
		}

		// Завершаем соединение
		beast::error_code ec;
		stream.socket().shutdown(tcp::socket::shutdown_both, ec);

		if(ec && ec != beast::errc::not_connected)
		{
			Error("ec && ec != beast::errc::not_connected");
			throw beast::system_error{ec};
		}

	} catch (const std::exception& e) {
		Error(QString("HttpClient exception: ") + QString::fromLocal8Bit(e.what()));
	}
	running = false;
	Log("HttpClient::run end");

	QMetaObject::invokeMethod(&invoker, [this]() {
		clientThread.join();
	});
}
