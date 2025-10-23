#include "http_server.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include "html_tamplates.h"

HTTPSession::HTTPSession(tcp::socket socket, DatabaseManager *dbManager) :
socket_(std::move(socket)),
dbManager_(dbManager) {
}

void HTTPSession::start() {
    readRequest();
}

void HTTPSession::readRequest() {
    auto self = shared_from_this();

    http::async_read(socket_, buffer_, request_,
            [self](beast::error_code ec, std::size_t bytes_transferred) {
                if (!ec) {
                    self->processRequest();
                }
            });
}

void HTTPSession::processRequest() {
    // Игнорируем запросы к favicon.ico
    if (request_.target() == "/favicon.ico") {
        std::cout << "HTTPSession::processRequest: Ignoring favicon request" << std::endl;
        sendResponse("", http::status::no_content);
        return;
    }

    // Игнорируем OPTIONS запросы (CORS preflight)
    if (request_.method() == http::verb::options) {
        std::cout << "HTTPSession::processRequest: Ignoring OPTIONS request" << std::endl;
        sendResponse("", http::status::no_content);
        return;
    }

    // Обрабатываем только корневой путь
    if (request_.target() != "/") {
        std::cout << " HTTPSession::processRequest: Unknown target: " << request_.target()
                  << std::endl;
        sendResponse("Not found", http::status::not_found);
        return;
    }

    switch (request_.method()) {
        case http::verb::get:
            handleGet();
            break;
        case http::verb::post:
            handlePost();
            break;
        default:
            sendResponse("Method not allowed", http::status::method_not_allowed);
    }
}

void HTTPSession::handleGet() {
    std::string html = createSearchPage();
    sendResponse(html);
}

void HTTPSession::handlePost() {
    try {
        std::string body = beast::buffers_to_string(request_.body().data());
        std::string query = parseFormData(body);

        // std::cout << "HTTPSession::processRequest: Search query: " << query << std::endl;

        auto words = parseQuery(query);
        if (words.empty() || words.size() > 4) {
            sendResponse(createErrorPage("Query must contain 1-4 words"),
                    http::status::bad_request);
            return;
        }

        for (auto &val : words) {
            std::cout << val << std::endl;
        }

        std::map<int, std::string, std::greater<int> > results;
        dbManager_->searchWords(results, words);
        if (results.empty()) {
            sendResponse(createErrorPage("Not found"),
                    http::status::bad_request);
            return;
        }
        // for (auto &val : results) {
        //     std::cout << val.first << " " << val.second << std::endl;
        // }

        // std::string test_html = R"(
        //     <!DOCTYPE html>
        //     <html>
        //     <head><title>Search Results</title></head>
        //     <body>
        //         <h1>Search Results</h1>
        //         <p>Query: )" + query + R"(</p>
        //         <p>Search functionality not implemented yet</p>
        //         <a href="/">New search</a>
        //     </body>
        //     </html>
        // )";

        std::string html = createResultsPage(results, query);
        sendResponse(html);

    } catch (const std::exception &e) {
        std::cerr << "HTTPSession::handlePost: Error processing POST request: " << e.what()
                  << std::endl;
        sendResponse(createErrorPage("Internal server error"), http::status::internal_server_error);
    }
}

void HTTPSession::sendResponse(const std::string &content, http::status status) {
    response_.result(status);
    response_.set(http::field::server, "Search Engine");
    response_.set(http::field::content_type, "text/html; charset=utf-8");
    beast::ostream(response_.body()) << content;

    response_.prepare_payload();

    auto self = shared_from_this();
    http::async_write(socket_, response_,
            [self](beast::error_code ec, std::size_t bytes_transferred) {
                if (!ec) {
                    std::cout
                            << "HTTPSession::sendResponse: Response sent successfully for session: "
                            << self.get() << std::endl;
                } else {
                    std::cerr << "HTTPSession::sendResponse:: Write error: " << ec.message()
                              << std::endl;
                }

                beast::error_code shutdown_ec;
                self->socket_.shutdown(tcp::socket::shutdown_send, shutdown_ec);

                if (shutdown_ec && shutdown_ec != beast::errc::not_connected) {
                    std::cerr << "HTTPSession::sendResponse: Shutdown error: "
                              << shutdown_ec.message() << std::endl;
                }

                // self->socket_.shutdown(tcp::socket::shutdown_send, ec);
            });
}

std::string HTTPSession::parseFormData(const std::string &body) {
    if (body.find("query=") == 0) {
        std::string encoded_query = body.substr(6);

        std::string decoded;
        for (size_t i = 0; i < encoded_query.size(); ++i) {
            if (encoded_query[i] == '+') {
                decoded += ' ';
            } else if (encoded_query[i] == '%' && i + 2 < encoded_query.size()) {
                decoded += encoded_query[i];
            } else {
                decoded += encoded_query[i];
            }
        }
        return decoded;
    }
    return "";
}

std::vector<std::string> HTTPSession::parseQuery(const std::string &query) {
    std::vector<std::string> words;
    std::stringstream ss(query);
    std::string word;

    while (ss >> word) {
        word.erase(std::remove_if(word.begin(), word.end(), [](char c) { return std::ispunct(c); }),
                word.end());

        std::transform(word.begin(), word.end(), word.begin(), ::tolower);

        if (!word.empty()) {
            words.push_back(word);
        }
    }

    return words;
}

HTTPServer::HTTPServer(net::io_context &ioc, const tcp::endpoint &endpoint,
        DatabaseManager *dbManager) :
ioc_(ioc),
acceptor_(ioc),
dbManager_(dbManager) {
    beast::error_code ec;

    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
        throw std::runtime_error("Failed to open acceptor: " + ec.message());
    }

    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) {
        throw std::runtime_error("Failed to set reuse address: " + ec.message());
    }

    acceptor_.bind(endpoint, ec);
    if (ec) {
        throw std::runtime_error("Failed to bind: " + ec.message());
    }

    acceptor_.listen(net::socket_base::max_listen_connections, ec);
    if (ec) {
        throw std::runtime_error("Failed to listen: " + ec.message());
    }
}

void HTTPServer::start() {
    acceptConnection();
    std::cout << "HTTPServer::start: HTTP Server started and listening..." << std::endl;
}

void HTTPServer::acceptConnection() {
    acceptor_.async_accept([this](beast::error_code ec, tcp::socket socket) {
        if (!ec) {
            std::make_shared<HTTPSession>(std::move(socket), dbManager_)->start();
        } else {
            std::cerr << "HTTPServer::acceptConnection: Accept error: " << ec.message()
                      << std::endl;
        }

        acceptConnection();
    });
}
