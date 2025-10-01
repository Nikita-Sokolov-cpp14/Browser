#include "spider.h"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <boost/beast/ssl.hpp>
#include <boost/asio/ssl.hpp>

#include "../utils/secondary_function.h"

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http; // from <boost/beast/http.hpp>
namespace net = boost::asio; // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp; // from <boost/asio/ip/tcp.hpp>

namespace {

const int maxRecursiveCount = 2;

} // namespace

SimpleHttpClient::SimpleHttpClient() :
    resolver_(ioc_),
    ssl_ctx_(ssl::context::tlsv12_client)
{
    // Настройка SSL контекста
    ssl_ctx_.set_default_verify_paths();
    ssl_ctx_.set_verify_mode(ssl::verify_none); // Для тестирования, в продакшене используйте verify_peer
}

SimpleHttpClient::~SimpleHttpClient() {
    // Закрываем соединения при разрушении объекта
    if (http_stream_ && http_stream_->socket().is_open()) {
        beast::error_code ec;
        http_stream_->socket().shutdown(tcp::socket::shutdown_both, ec);
    }
    if (https_stream_ && beast::get_lowest_layer(*https_stream_).socket().is_open()) {
        beast::error_code ec;
        beast::get_lowest_layer(*https_stream_).close();
    }
}

std::string SimpleHttpClient::get(const RequestConfig &reqConfig, int max_redirects = 5) {
    std::cout << "SimpleHttpClient::get " << reqConfig.host << ":" << reqConfig.port << reqConfig.target << std::endl;

    if (max_redirects <= 0) {
        throw std::runtime_error("Too many redirects");
    }

    // Выбираем метод в зависимости от схемы
    if (reqConfig.port == "443") {
        return perform_https_request(reqConfig, max_redirects);
    } else {
        return perform_http_request(reqConfig, max_redirects);
    }
}

std::string SimpleHttpClient::perform_http_request(const RequestConfig &reqConfig, int max_redirects) {
    // Создаем HTTP поток
    http_stream_ = std::make_unique<beast::tcp_stream>(ioc_);

    // Разрешение домена
    auto const results = resolver_.resolve(reqConfig.host, reqConfig.port);
    http_stream_->connect(results);

    // Создание запроса
    http::request<http::string_body> req{http::verb::get, reqConfig.target, 11};
    req.set(http::field::host, reqConfig.host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set(http::field::accept, "*/*");

    // Отправка запроса
    http::write(*http_stream_, req);

    // Получение ответа
    beast::flat_buffer buffer;
    http::response<http::dynamic_body> res;
    http::read(*http_stream_, buffer, res);

    std::string response = beast::buffers_to_string(res.body().data());

    // Обработка редиректов
    if (is_redirect(res.result())) {
        auto location = res.find(http::field::location);
        if (location != res.end()) {
            std::string redirect_url = location->value().to_string();
            std::cout << "Redirecting to: " << redirect_url << std::endl;

            // Закрываем соединение
            beast::error_code ec;
            http_stream_->socket().shutdown(tcp::socket::shutdown_both, ec);
            http_stream_.reset();

            return handle_redirect(redirect_url, max_redirects - 1);
        }
    }

    // Закрываем соединение
    beast::error_code ec;
    http_stream_->socket().shutdown(tcp::socket::shutdown_both, ec);
    http_stream_.reset();

    return response;
}

std::string SimpleHttpClient::perform_https_request(const RequestConfig &reqConfig, int max_redirects) {
    // Создаем HTTPS поток
    https_stream_ = std::make_unique<beast::ssl_stream<beast::tcp_stream>>(ioc_, ssl_ctx_);

    // Устанавливаем SNI Hostname (важно для многих серверов)
    if (!SSL_set_tlsext_host_name(https_stream_->native_handle(), reqConfig.host.c_str())) {
        beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
        throw beast::system_error{ec};
    }

    // Разрешение домена
    auto const results = resolver_.resolve(reqConfig.host, reqConfig.port);

    // Устанавливаем соединение
    beast::get_lowest_layer(*https_stream_).connect(results);

    // Выполняем SSL handshake
    https_stream_->handshake(ssl::stream_base::client);

    // Создание запроса
    http::request<http::string_body> req{http::verb::get, reqConfig.target, 11};
    req.set(http::field::host, reqConfig.host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set(http::field::accept, "*/*");

    // Отправка запроса
    http::write(*https_stream_, req);

    // Получение ответа
    beast::flat_buffer buffer;
    http::response<http::dynamic_body> res;
    http::read(*https_stream_, buffer, res);

    std::string response = beast::buffers_to_string(res.body().data());

    // Обработка редиректов
    if (is_redirect(res.result())) {
        auto location = res.find(http::field::location);
        if (location != res.end()) {
            std::string redirect_url = location->value().to_string();
            std::cout << "Redirecting to: " << redirect_url << std::endl;

            // Закрываем соединение
            beast::error_code ec;
            https_stream_->shutdown(ec);
            https_stream_.reset();

            return handle_redirect(redirect_url, max_redirects - 1);
        }
    }

    // Закрываем соединение
    beast::error_code ec;
    https_stream_->shutdown(ec);
    https_stream_.reset();

    return response;
}

std::string SimpleHttpClient::handle_redirect(const std::string& redirect_url, int max_redirects) {
    // Парсим URL редиректа и делаем новый запрос
    RequestConfig config = parseUrl(redirect_url);
    return get(config, max_redirects);
}

bool SimpleHttpClient::is_redirect(http::status status) {
    return status == http::status::moved_permanently || status == http::status::found ||
            status == http::status::see_other || status == http::status::temporary_redirect ||
            status == http::status::permanent_redirect;
}

Spider::Spider() :
dbmanager_(nullptr),
indexer_() {
}

void Spider::connectDb(DatabaseManager *dbManager) {
    dbmanager_ = dbManager;
    dbmanager_->clearDatabase();
}

void Spider::process() {
    while (!queueReferences_.empty()) {
        const QueueParams &queueParams = queueReferences_.front();
        std::cout << "host: " << queueParams.requestConfig.host
                  << " port: " << queueParams.requestConfig.port
                  << " target: " << queueParams.requestConfig.target << std::endl;
        std::string responseStr = client_.get(queueParams.requestConfig);
        // std::cout << responseStr << std::endl;
        indexer_.setPage(responseStr, queueParams.requestConfig.host, *dbmanager_);
        std::vector<RequestConfig> configs;
        extractAllLinks(responseStr, configs);
        for (auto config : configs) {
            if (queueParams.recursiveCount < maxRecursiveCount) {
                queueReferences_.push(QueueParams(config, queueParams.recursiveCount + 1));
            }
        }
        std::cout << "---" << std::endl;
        queueReferences_.pop();
    }
}

void Spider::start(const RequestConfig &startRequestConfig) {
    queueReferences_.push(QueueParams(startRequestConfig, 1));
    process();
}
