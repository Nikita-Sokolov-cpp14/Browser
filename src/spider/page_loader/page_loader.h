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

    std::string get(const RequestConfig &reqConfig, int maxRedirects = 5);

private:
    struct RequestContext {
        const RequestConfig &config;
        int maxRedirects;
        std::chrono::seconds timeout {10}; // Таймаут по умолчанию 10 секунд
    };

    template<typename Stream>
    void setupTimeouts(Stream &stream, const RequestContext &ctx) {
        stream.expires_after(ctx.timeout);
    }

    bool is_redirect(http::status status);

    // Основной метод выполнения запроса
    std::string performRequest(const RequestContext &ctx);

    // Методы для разных протоколов
    std::string performHttpRequest(const RequestContext &ctx);
    std::string performHttpsRequest(const RequestContext &ctx);

    // Обработка редиректов
    std::string handleRedirect(const std::string &redirectUrl, int maxRedirects);

    net::io_context ioc_;
    tcp::resolver resolver_;
    std::unique_ptr<beast::tcp_stream> httpStream_;
    ssl::context sslCtx_;
    std::unique_ptr<beast::ssl_stream<beast::tcp_stream> > httpsStream_;
};
