#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>


namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http; // from <boost/beast/http.hpp>
namespace net = boost::asio; // from <boost/asio.hpp>
using tcp = net::ip::tcp; // from <boost/asio/ip/tcp.hpp>

void parseHTML(const std::string& html);

void parse(std::string& page);

// Performs an HTTP GET and prints the response
int main(int argc, char **argv) {
    try {
        // Check command line arguments.
        if (argc != 4 && argc != 5) {
            std::cerr
                    << "Usage: http-client-sync <host> <port> <target> [<HTTP version: 1.0 or 1.1(default)>]\n"
                    << "Example:\n"
                    << "    http-client-sync www.example.com 80 /\n"
                    << "    http-client-sync www.example.com 80 / 1.0\n";
            return EXIT_FAILURE;
        }
        auto const host = argv[1];
        auto const port = argv[2];
        auto const target = argv[3];
        int version = argc == 5 && !std::strcmp("1.0", argv[4]) ? 10 : 11;

        // The io_context is required for all I/O
        net::io_context ioc;

        // These objects perform our I/O
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);

        // Look up the domain name
        auto const results = resolver.resolve(host, port);

        // Make the connection on the IP address we get from a lookup
        stream.connect(results);

        // Set up an HTTP GET request message
        http::request<http::string_body> req {http::verb::get, target, version};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // Send the HTTP request to the remote host
        http::write(stream, req);

        // This buffer is used for reading and must be persisted
        beast::flat_buffer buffer;

        // Declare a container to hold the response
        http::response<http::dynamic_body> res;

        // Receive the HTTP response
        http::read(stream, buffer, res);

        // Write the message to standard out
        // std::cout << res << std::endl;
        std::string response_str = boost::beast::buffers_to_string(res.body().data());
        parse(response_str);

        // Gracefully close the socket
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        // not_connected happens sometimes
        // so don't bother reporting it.
        //
        if (ec && ec != beast::errc::not_connected)
            throw beast::system_error {ec};

        // If we get here then the connection is closed gracefully
    } catch (std::exception const &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void parse(std::string& page) {
    // std::cout << page << std::endl;
    parseHTML(page);
}

void parseHTML(const std::string& html) {
    // Парсим HTML строку
    htmlDocPtr doc = htmlReadMemory(
        html.c_str(),        // исходная строка
        html.length(),       // длина строки
        nullptr,             // URL (не используется)
        nullptr,             // кодировка (автоопределение)
        HTML_PARSE_NOERROR   // опции парсинга
    );

    if (!doc) {
        std::cerr << "Ошибка при парсинге HTML" << std::endl;
        return;
    }

    // Создаем контекст XPath
    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
    if (!xpathCtx) {
        xmlFreeDoc(doc);
        std::cerr << "Ошибка создания контекста XPath" << std::endl;
        return;
    }

    try {
        // Пример использования XPath для поиска элементов
        // Находим все теги <div>
        const xmlChar* xpathExpr = (const xmlChar*)"//div";
        xmlXPathObjectPtr result = xmlXPathEvalExpression(xpathExpr, xpathCtx);

        if (result && result->nodesetval) {
            // Перебираем найденные узлы
            for (int i = 0; i < result->nodesetval->nodeNr; i++) {
                xmlNodePtr node = result->nodesetval->nodeTab[i];

                // Получаем текст узла
                xmlChar* content = xmlNodeGetContent(node);
                if (content) {
                    std::cout << "Текст узла: " << content << std::endl;
                    xmlFree(content);
                }

                // Пример получения атрибута
                xmlChar* attrValue = xmlGetProp(node, (const xmlChar*)"class");
                if (attrValue) {
                    std::cout << "Атрибут class: " << attrValue << std::endl;
                    xmlFree(attrValue);
                }
            }
        }
        xmlXPathFreeObject(result);

    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
    }

    // Освобождаем ресурсы
    xmlXPathFreeContext(xpathCtx);
    xmlFreeDoc(doc);
}
