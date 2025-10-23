#pragma once

#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <string>
#include "../database_manager/database_manager.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

/**
* @brief HTTP сессия.
*/
class HTTPSession : public std::enable_shared_from_this<HTTPSession> {
public:
    /**
    * @brief Конструктор.
    * @param socket Сокет.
    * @param dbManager Указатель на класс менеджера БД.
    */
    HTTPSession(tcp::socket socket, DatabaseManager *dbManager);

    /**
    * @brief Запустить сессию.
    */
    void start();

private:
    /**
    * @brief Прочитать запрос.
    */
    void readRequest();

    /**
    * @brief Обработать запрос.
    */
    void processRequest();

    /**
    * @brief Обработать GET запрос.
    */
    void handleGet();

    /**
    * @brief Обработать POST запрос.
    */
    void handlePost();

    /**
    * @brief Отправить ответ на запрос.
    * @param content Строка с HTML кодом страницы-ответа.
    * @param status Статус.
    */
    void sendResponse(const std::string &content, http::status status = http::status::ok);

    /**
    * @brief Получить слова для поиска из строки с запросом.
    * @param body Строка с HTML кодом запроса.
    * @return Строка со словами запроса.
    */
    std::string parseFormData(const std::string &body);

    /**
    * @brief Разить строку со словами на отдельные строки.
    * @param query Строка со словами.
    * @return Вектор из отдельных слов запроса.
    */
    std::vector<std::string> parseQuery(const std::string &query);

    tcp::socket socket_; //!< Сокет.
    beast::flat_buffer buffer_ {8192}; //!< Буффер.
    http::request<http::dynamic_body> request_; //!< Запрос.
    http::response<http::dynamic_body> response_; //!< Ответ.
    DatabaseManager *dbManager_; //!< Объект взаимодействия с psql.
};

/**
* @brief HTTP сервер.
*/
class HTTPServer {
public:
    HTTPServer(net::io_context &ioc, const tcp::endpoint &endpoint, DatabaseManager *dbManager);

    /**
    * @brief Запустить сервер.
    */
    void start();

private:
    void acceptConnection();

    net::io_context &ioc_;
    tcp::acceptor acceptor_;
    DatabaseManager *dbManager_;
};
