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

class HTTPSession : public std::enable_shared_from_this<HTTPSession> {
public:
    HTTPSession(tcp::socket socket, DatabaseManager *dbManager);
    void start();

private:
    void read_request();
    void process_request();
    void handle_get();
    void handle_post();
    void send_response(const std::string &content, http::status status = http::status::ok);
    std::string parse_form_data(const std::string &body);
    std::vector<std::string> parse_query(const std::string &query);

    tcp::socket socket_;
    beast::flat_buffer buffer_ {8192};
    http::request<http::dynamic_body> request_;
    http::response<http::dynamic_body> response_;
    DatabaseManager *dbManager_;
};

class HTTPServer {
public:
    HTTPServer(net::io_context &ioc, const tcp::endpoint &endpoint, DatabaseManager *dbManager);
    void start();

private:
    void accept_connection();

    net::io_context &ioc_;
    tcp::acceptor acceptor_;
    DatabaseManager *dbManager_;
};

// HTML templates
// std::string create_search_page();
// std::string create_results_page(const std::vector<SearchResult>& results, const std::string& query);
// std::string create_error_page(const std::string& message);
