#include "../common_data.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/ssl.hpp>

#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp;

class SimpleHttpClient {
public:
    SimpleHttpClient();
    ~SimpleHttpClient();

    std::string get(const RequestConfig &reqConfig, int max_redirects = 5);

private:
    struct RequestContext {
        const RequestConfig& config;
        int max_redirects;
        std::chrono::seconds timeout{10};  // Таймаут по умолчанию 10 секунд
    };

    bool is_redirect(http::status status);

    // Основной метод выполнения запроса
    std::string perform_request(const RequestContext& ctx);

    // Методы для разных протоколов
    std::string perform_http_request(const RequestContext& ctx);
    std::string perform_https_request(const RequestContext& ctx);

    // Обработка редиректов
    std::string handle_redirect(const std::string& redirect_url, int max_redirects);

    // Установка таймаутов с обработкой
    template<typename Stream>
    void setup_timeouts(Stream& stream, const RequestContext& ctx);

    // Парсинг URL с обработкой ошибок
    RequestConfig parse_url_safe(const std::string& url);

    net::io_context ioc_;
    tcp::resolver resolver_;
    std::unique_ptr<beast::tcp_stream> http_stream_;
    ssl::context ssl_ctx_;
    std::unique_ptr<beast::ssl_stream<beast::tcp_stream>> https_stream_;
};
