#pragma once

#include "../common_data.h"

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

class SimpleHttpClient {
public:
    SimpleHttpClient();
    ~SimpleHttpClient();

    std::string get(const RequestConfig &reqConfig, int max_redirects = 5);

private:
    bool is_redirect(http::status status);

    // Методы для выполнения HTTP и HTTPS запросов
    std::string perform_http_request(const RequestConfig &reqConfig, int max_redirects);
    std::string perform_https_request(const RequestConfig &reqConfig, int max_redirects);
    std::string handle_redirect(const std::string& redirect_url, int max_redirects);

    // Метод для установки таймаутов
    void set_timeouts(beast::tcp_stream& stream);
    void set_ssl_timeouts(beast::ssl_stream<beast::tcp_stream>& stream);

    net::io_context ioc_;
    tcp::resolver resolver_;
    std::unique_ptr<beast::tcp_stream> http_stream_;
    ssl::context ssl_ctx_;
    std::unique_ptr<beast::ssl_stream<beast::tcp_stream>> https_stream_;
};
