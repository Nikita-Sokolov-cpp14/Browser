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

std::string getPage(const RequestConfig &startRequestConfig) {
    try {
        std::string responseStr;

        // std::cout << "--- target =" << startRequestConfig.target << std::endl;

        // Check command line arguments.
        if (startRequestConfig.host == "" || startRequestConfig.port == "" ||
                startRequestConfig.target == "") {
            std::cerr
                    << "Usage: http-client-sync <host> <port> <target> [<HTTP version: 1.0 or 1.1(default)>]\n"
                    << "Example:\n"
                    << "    http-client-sync www.example.com 80 /\n"
                    << "    http-client-sync www.example.com 80 / 1.0\n";
            return "";
        }
        // int version = argc == 5 && !std::strcmp("1.0", argv[4]) ? 10 : 11;
        //! TODO: Исправить
        int version = 11;

        // The io_context is required for all I/O
        net::io_context ioc;

        // These objects perform our I/O
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);

        // Look up the domain name
        auto const results = resolver.resolve(startRequestConfig.host, startRequestConfig.port);

        // Make the connection on the IP address we get from a lookup
        stream.connect(results);

        // Set up an HTTP GET request message
        http::request<http::string_body> req {http::verb::get, startRequestConfig.target, version};
        req.set(http::field::host, startRequestConfig.host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // Send the HTTP request to the remote host
        http::write(stream, req);

        // This buffer is used for reading and must be persisted
        beast::flat_buffer buffer;

        // Declare a container to hold the response
        http::response<http::dynamic_body> res;

        // Receive the HTTP response
        http::read(stream, buffer, res);

        responseStr = boost::beast::buffers_to_string(res.body().data());

        // Gracefully close the socket
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        // not_connected happens sometimes
        // so don't bother reporting it.
        //
        if (ec && ec != beast::errc::not_connected)
            throw beast::system_error {ec};
        return responseStr;
        // If we get here then the connection is closed gracefully
    } catch (std::exception const &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return "";
    }
    return "";
}

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
        std::string responseStr = getPage(queueParams.requestConfig);
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
