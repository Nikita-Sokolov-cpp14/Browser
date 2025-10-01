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
stream_(ioc_) {
}

std::string SimpleHttpClient::get(const RequestConfig &reqConfig, int max_redirects = 5) {
    std::cout << "SimpleHttpClient::get " << reqConfig.host << reqConfig.port << reqConfig.target << std::endl;

    if (max_redirects <= 0) {
        throw std::runtime_error("Too many redirects");
    }

    // Разрешение домена
    auto const results = resolver_.resolve(reqConfig.host, reqConfig.port);
    stream_.connect(results);

    // Создание запроса
    http::request<http::string_body> req {http::verb::get, reqConfig.target, 11};
    req.set(http::field::host, reqConfig.host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set(http::field::accept, "*/*");

    // Отправка запроса
    http::write(stream_, req);

    // Получение ответа
    beast::flat_buffer buffer;
    http::response<http::dynamic_body> res;
    http::read(stream_, buffer, res);

    // Обработка редиректов
    if (is_redirect(res.result()) && max_redirects > 0) {
        auto location = res.find(http::field::location);
        if (location != res.end()) {
            std::string redirect_url = location->value().to_string();
            std::cout << "Redirecting to: " << redirect_url << std::endl;

            // Закрываем соединение
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // Парсим URL и делаем рекурсивный вызов
            RequestConfig config = parseUrl(redirect_url);
            return get(config, max_redirects - 1);
        }
    }

    std::string response = beast::buffers_to_string(res.body().data());

    // Закрываем соединение
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

    return response;
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
        std::cout << responseStr << std::endl;
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
