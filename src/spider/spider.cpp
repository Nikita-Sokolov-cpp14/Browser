#include "spider.h"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <boost/beast/ssl.hpp>
#include <boost/asio/ssl.hpp>

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http; // from <boost/beast/http.hpp>
namespace net = boost::asio; // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp; // from <boost/asio/ip/tcp.hpp>

namespace {

const int maxRecursiveCount = 2;

} // namespace

RequestConfig parseUrl(const std::string &url) {
    RequestConfig config;
    std::cout << url << std::endl;

    // Если ссылка пустая, возвращаем пустую структуру
    if (url.empty()) {
        std::cerr << "parseUrl: url is empty" << std::endl;
        return config;
    }

    // Находим позицию начала хоста (после ://)
    size_t scheme_end = url.find("://");
    if (scheme_end == std::string::npos) {
        return config;
    }
    std::string scheme = url.substr(0, scheme_end);
    size_t host_start = scheme_end + 3; // пропускаем "://"

    // Находим конец хоста (до порта или пути)
    size_t host_end = url.find_first_of('/', host_start);
    std::string hostPort = url.substr(host_start, host_end - host_start);
    size_t portStart = hostPort.find_last_of(':');

    bool portIsExist = false;

    if (portStart == hostPort.npos) {
        config.host = hostPort;
    } else {
        config.host = hostPort.substr(0, portStart);
        portStart++; // учитываем :
        config.port = hostPort.substr(portStart, host_end - portStart);
        portIsExist = true;
    }
    config.target = url.substr(host_end);

    if (!portIsExist) {
        // std::cout << scheme << std::endl;
        if (scheme == "https") {
            config.port = "443";
        } else {
            config.port = "80";
        }
    }

    // std::cout << config.host << std::endl;
    // std::cout << config.port << std::endl;
    // std::cout << config.target << std::endl;
    return config;
}

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

void extractAllLinks(const std::string &htmlContent, std::vector<RequestConfig> &links) {
    htmlDocPtr doc = htmlReadDoc(reinterpret_cast<const xmlChar *>(htmlContent.c_str()), nullptr,
            nullptr, HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);

    if (doc == nullptr) {
        std::cerr << "extractAllLinks: Can't parsing HTML" << std::endl;
        return;
    }

    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
    if (xpathCtx == nullptr) {
        std::cerr << "extractAllLinks: Error creating XPath context" << std::endl;
        xmlFreeDoc(doc);
        return;
    }

    // Более специфичный XPath для тегов <a>
    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression(BAD_CAST "//a/@href", xpathCtx);

    if (xpathObj == nullptr || xpathObj->nodesetval == nullptr) {
        std::cerr << "extractAllLinks: Error XPath request or ref not found" << std::endl;
        xmlXPathFreeContext(xpathCtx);
        xmlFreeDoc(doc);
        return;
    }

    int size = xpathObj->nodesetval->nodeNr;
    for (int i = 0; i < size; ++i) {
        xmlNodePtr attrNode = xpathObj->nodesetval->nodeTab[i];

        if (attrNode->type == XML_ATTRIBUTE_NODE) {
            xmlChar *hrefValue = xmlNodeGetContent(attrNode);
            if (hrefValue != nullptr) {
                std::string url = reinterpret_cast<const char *>(hrefValue);
                RequestConfig config = parseUrl(url);

                std::cout << url << std::endl;

                if (!config.host.empty()) {
                    links.push_back(config);
                }
                xmlFree(hrefValue);
            }
        }
    }

    xmlXPathFreeObject(xpathObj);
    xmlXPathFreeContext(xpathCtx);
    xmlFreeDoc(doc);
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
