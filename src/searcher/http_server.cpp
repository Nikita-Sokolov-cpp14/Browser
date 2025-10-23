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
    read_request();
}

void HTTPSession::read_request() {
    auto self = shared_from_this();

    http::async_read(socket_, buffer_, request_,
            [self](beast::error_code ec, std::size_t bytes_transferred) {
                if (!ec) {
                    self->process_request();
                }
            });
}

void HTTPSession::process_request() {
    // Игнорируем запросы к favicon.ico
    if (request_.target() == "/favicon.ico") {
        std::cout << "Ignoring favicon request" << std::endl;
        send_response("", http::status::no_content);
        return;
    }

    // Игнорируем OPTIONS запросы (CORS preflight)
    if (request_.method() == http::verb::options) {
        std::cout << "Ignoring OPTIONS request" << std::endl;
        send_response("", http::status::no_content);
        return;
    }

    // Обрабатываем только корневой путь
    if (request_.target() != "/") {
        std::cout << "Unknown target: " << request_.target() << std::endl;
        send_response("Not found", http::status::not_found);
        return;
    }

    switch (request_.method()) {
        case http::verb::get:
            handle_get();
            break;
        case http::verb::post:
            handle_post();
            break;
        default:
            send_response("Method not allowed", http::status::method_not_allowed);
    }
}

void HTTPSession::handle_get() {
    std::string html = create_search_page();
    send_response(html);
}

void HTTPSession::handle_post() {
    try {
        // Parse form data
        std::string body = beast::buffers_to_string(request_.body().data());
        std::string query = parse_form_data(body);

        std::cout << "Search query: " << query << std::endl;

        // Parse query into words
        auto words = parse_query(query);
        if (words.empty() || words.size() > 4) {
            // send_response(create_error_page("Query must contain 1-4 words"),
            //         http::status::bad_request);
            return;
        }

        std::map<int, std::string, std::greater<int>> results;
        dbManager_->searchWords(results, words);
        for (auto &val : results) {
            std::cout << val.first << " " << val.second << std::endl;
        }

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

        std::string html = create_results_page(results, query);
        send_response(html);

    } catch (const std::exception &e) {
        std::cerr << "Error processing POST request: " << e.what() << std::endl;
        // send_response(create_error_page("Internal server error"),
        //         http::status::internal_server_error);
    }
}

void HTTPSession::send_response(const std::string &content, http::status status) {
    response_.result(status);
    response_.set(http::field::server, "Search Engine");
    response_.set(http::field::content_type, "text/html; charset=utf-8");
    beast::ostream(response_.body()) << content;

    response_.prepare_payload();

    auto self = shared_from_this();
    http::async_write(socket_, response_,
            [self](beast::error_code ec, std::size_t bytes_transferred) {
                if (!ec) {
                    std::cout << "Response sent successfully for session: " << self.get()
                              << std::endl;
                } else {
                    std::cerr << "Write error: " << ec.message() << std::endl;
                }

                beast::error_code shutdown_ec;
                self->socket_.shutdown(tcp::socket::shutdown_send, shutdown_ec);

                if (shutdown_ec && shutdown_ec != beast::errc::not_connected) {
                    std::cerr << "Shutdown error: " << shutdown_ec.message() << std::endl;
                }

                // self->socket_.shutdown(tcp::socket::shutdown_send, ec);
            });
}

std::string HTTPSession::parse_form_data(const std::string &body) {
    // Simple form parsing for "query=search+terms"
    if (body.find("query=") == 0) {
        std::string encoded_query = body.substr(6); // Remove "query="

        // Basic URL decoding
        std::string decoded;
        for (size_t i = 0; i < encoded_query.size(); ++i) {
            if (encoded_query[i] == '+') {
                decoded += ' ';
            } else if (encoded_query[i] == '%' && i + 2 < encoded_query.size()) {
                // Skip % decoding for simplicity
                decoded += encoded_query[i];
            } else {
                decoded += encoded_query[i];
            }
        }
        return decoded;
    }
    return "";
}

std::vector<std::string> HTTPSession::parse_query(const std::string &query) {
    std::vector<std::string> words;
    std::stringstream ss(query);
    std::string word;

    while (ss >> word) {
        // Remove punctuation (basic)
        word.erase(std::remove_if(word.begin(), word.end(), [](char c) { return std::ispunct(c); }),
                word.end());

        // Convert to lowercase
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

    // Open the acceptor
    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
        throw std::runtime_error("Failed to open acceptor: " + ec.message());
    }

    // Allow address reuse
    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) {
        throw std::runtime_error("Failed to set reuse address: " + ec.message());
    }

    // Bind to the server address
    acceptor_.bind(endpoint, ec);
    if (ec) {
        throw std::runtime_error("Failed to bind: " + ec.message());
    }

    // Start listening for connections
    acceptor_.listen(net::socket_base::max_listen_connections, ec);
    if (ec) {
        throw std::runtime_error("Failed to listen: " + ec.message());
    }
}

void HTTPServer::start() {
    accept_connection();
    std::cout << "HTTP Server started and listening..." << std::endl;
}

void HTTPServer::accept_connection() {
    acceptor_.async_accept([this](beast::error_code ec, tcp::socket socket) {
        if (!ec) {
            std::make_shared<HTTPSession>(std::move(socket), dbManager_)->start();
        } else {
            std::cerr << "Accept error: " << ec.message() << std::endl;
        }

        // Continue accepting new connections
        accept_connection();
    });
}
