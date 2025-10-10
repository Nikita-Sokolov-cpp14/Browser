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

/**
* @brief Класс, который скачивает HTML страницу.
*/
class PageLoader {
public:
    /**
    * @brief Конструктор.
    */
    PageLoader();

    /**
    * @brief Деструктор.
    */
    ~PageLoader();

    /**
    * @brief Скачать HTML страницу.
    * @param reqConfig Параметры запроса.
    * @param countRedirects Число редиректов.
    * @return Строка с содержимым HTML страницы.
    */
    std::string get(const RequestConfig &reqConfig, int countRedirects = 5);

private:
    /**
    * @brief Контекст запроса HTML страницы.
    */
    struct RequestContext {
        const RequestConfig &config; //!< Параметры запроса.
        int countRedirects; //!< Число редиректов.
        std::chrono::seconds timeout {10}; //!< Таймаут (по умолчанию 10 секунд).
    };

    /**
    * @brief Установить время ожидания ответа на запрос.
    * @tparam Stream Тип потока.
    * @param stream Поток.
    * @param ctx Контекст запроса.
    */
    template<typename Stream>
    void setupTimeouts(Stream &stream, const RequestContext &ctx) {
        stream.expires_after(ctx.timeout);
    }

    /**
    * @brief Проверить, относится ли данный статус к редиректу.
    * @param status Статус.
    * @return true, если статус относится к редиректу. False - иначе.
    */
    bool isRedirect(http::status status);

    /**
    * @brief Выполнить запрос.
    * @param ctx Контекст запроса.
    * @return Строка с ответом на запрос.
    */
    std::string performRequest(const RequestContext &ctx);

    /**
    * @brief Выполнить запрос для протокола HTTP.
    * @param ctx Контекст запроса.
    * @return Строка с ответом на запрос.
    */
    std::string performHttpRequest(const RequestContext &ctx);

    /**
    * @brief Выполнить запрос для протокола HTTPS.
    * @param ctx Контекст запроса.
    * @return Строка с ответом на запрос.
    */
    std::string performHttpsRequest(const RequestContext &ctx);

    // Обработка редиректов
    /**
    * @brief Обработать редиректы.
    * @param redirectUrl URL редиректа.
    * @param countRedirects Число редиректов.
    * @return Строка с ответом на запрос.
    */
    std::string handleRedirect(const std::string &redirectUrl, int countRedirects);

    net::io_context ioc_;
    tcp::resolver resolver_;
    std::unique_ptr<beast::tcp_stream> httpStream_;
    ssl::context sslCtx_;
    std::unique_ptr<beast::ssl_stream<beast::tcp_stream> > httpsStream_;
};
