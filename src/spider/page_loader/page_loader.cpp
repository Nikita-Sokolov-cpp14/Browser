#include "page_loader.h"
#include "../utils/secondary_function.h"

#include <iostream>

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http; // from <boost/beast/http.hpp>
namespace net = boost::asio; // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp; // from <boost/asio/ip/tcp.hpp>

SimpleHttpClient::SimpleHttpClient() :
    resolver_(ioc_),
    ssl_ctx_(ssl::context::tls_client)
{
    // Более гибкая настройка SSL контекста
    ssl_ctx_.set_default_verify_paths();
    ssl_ctx_.set_verify_mode(ssl::verify_none);

    // Совместимость с разными серверами
    SSL_CTX_set_options(ssl_ctx_.native_handle(),
                       SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);

    // Поддержка SNI
    SSL_CTX_set_tlsext_servername_callback(ssl_ctx_.native_handle(), nullptr);
}

SimpleHttpClient::~SimpleHttpClient() {
    // Аккуратно закрываем соединения
    try {
        if (http_stream_ && http_stream_->socket().is_open()) {
            beast::error_code ec;
            http_stream_->socket().shutdown(tcp::socket::shutdown_both, ec);
        }
        if (https_stream_ && beast::get_lowest_layer(*https_stream_).socket().is_open()) {
            beast::error_code ec;
            beast::get_lowest_layer(*https_stream_).close();
        }
    } catch (...) {
        // Игнорируем исключения в деструкторе
    }
}

void SimpleHttpClient::set_timeouts(beast::tcp_stream& stream) {
    stream.expires_after(std::chrono::seconds(2));
}

void SimpleHttpClient::set_ssl_timeouts(beast::ssl_stream<beast::tcp_stream>& stream) {
    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(2));
}

std::string SimpleHttpClient::get(const RequestConfig &reqConfig, int max_redirects) {
    if (max_redirects <= 0) {
        throw std::runtime_error("Too many redirects");
    }

    if (reqConfig.port == "443") {
        return perform_https_request(reqConfig, max_redirects);
    } else {
        return perform_http_request(reqConfig, max_redirects);
    }
}

std::string SimpleHttpClient::perform_http_request(const RequestConfig &reqConfig, int max_redirects) {
    try {
        http_stream_ = std::make_unique<beast::tcp_stream>(ioc_);
        set_timeouts(*http_stream_);

        auto const results = resolver_.resolve(reqConfig.host, reqConfig.port);
        http_stream_->connect(results);

        http::request<http::string_body> req{http::verb::get, reqConfig.target, 11};
        req.set(http::field::host, reqConfig.host);
        req.set(http::field::user_agent, "Mozilla/5.0 (compatible; SimpleHttpClient)");
        req.set(http::field::accept, "*/*");
        req.set(http::field::accept_encoding, "gzip, deflate");

        std::cout << "Sending HTTP request to: " << reqConfig.host << reqConfig.target << std::endl;
        http::write(*http_stream_, req);

        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;
        http::read(*http_stream_, buffer, res);

        std::string response = beast::buffers_to_string(res.body().data());
        std::cout << "Received HTTP response: " << res.result() << std::endl;

        if (is_redirect(res.result())) {
            auto location = res.find(http::field::location);
            if (location != res.end()) {
                std::string redirect_url = location->value().to_string();
                // std::cout << "Redirecting from HTTP to: " << redirect_url << std::endl;

                beast::error_code ec;
                http_stream_->socket().shutdown(tcp::socket::shutdown_both, ec);
                http_stream_.reset();

                return handle_redirect(redirect_url, max_redirects - 1);
            }
        }

        beast::error_code ec;
        http_stream_->socket().shutdown(tcp::socket::shutdown_both, ec);
        http_stream_.reset();

        return response;
    } catch (const std::exception& e) {
        std::cerr << "HTTP request failed: " << e.what() << std::endl;
        if (http_stream_) {
            beast::error_code ec;
            http_stream_->socket().shutdown(tcp::socket::shutdown_both, ec);
            http_stream_.reset();
        }
        throw;
    }
}

std::string SimpleHttpClient::perform_https_request(const RequestConfig &reqConfig, int max_redirects) {
    try {
        https_stream_ = std::make_unique<beast::ssl_stream<beast::tcp_stream>>(ioc_, ssl_ctx_);
        set_ssl_timeouts(*https_stream_);

        // Важно: устанавливаем SNI перед подключением
        if (!SSL_set_tlsext_host_name(https_stream_->native_handle(), reqConfig.host.c_str())) {
            beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
            throw beast::system_error{ec};
        }

        auto const results = resolver_.resolve(reqConfig.host, reqConfig.port);

        std::cout << "Connecting to HTTPS: " << reqConfig.host << ":" << reqConfig.port << std::endl;
        beast::get_lowest_layer(*https_stream_).connect(results);

        std::cout << "Performing SSL handshake..." << std::endl;
        https_stream_->handshake(ssl::stream_base::client);
        std::cout << "SSL handshake successful" << std::endl;

        http::request<http::string_body> req{http::verb::get, reqConfig.target, 11};
        req.set(http::field::host, reqConfig.host);
        req.set(http::field::user_agent, "Mozilla/5.0 (compatible; SimpleHttpClient)");
        req.set(http::field::accept, "*/*");
        req.set(http::field::accept_encoding, "gzip, deflate");

        std::cout << "Sending HTTPS request to: " << reqConfig.host << reqConfig.target << std::endl;
        http::write(*https_stream_, req);

        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;
        http::read(*https_stream_, buffer, res);

        std::string response = beast::buffers_to_string(res.body().data());
        std::cout << "Received HTTPS response: " << res.result() << std::endl;

        if (is_redirect(res.result())) {
            auto location = res.find(http::field::location);
            if (location != res.end()) {
                std::string redirect_url = location->value().to_string();
                std::cout << "Redirecting from HTTPS to: " << redirect_url << std::endl;

                beast::error_code ec;
                https_stream_->shutdown(ec);
                https_stream_.reset();

                return handle_redirect(redirect_url, max_redirects - 1);
            }
        }

        beast::error_code ec;
        https_stream_->shutdown(ec);
        https_stream_.reset();

        return response;
    } catch (const std::exception& e) {
        std::cerr << "HTTPS request failed: " << e.what() << std::endl;
        if (https_stream_) {
            beast::error_code ec;
            https_stream_->shutdown(ec);
            https_stream_.reset();
        }
        throw;
    }
}

std::string SimpleHttpClient::handle_redirect(const std::string& redirect_url, int max_redirects) {
    RequestConfig config = parseUrl(redirect_url);
    return get(config, max_redirects);
}

bool SimpleHttpClient::is_redirect(http::status status) {
    return status == http::status::moved_permanently || status == http::status::found ||
            status == http::status::see_other || status == http::status::temporary_redirect ||
            status == http::status::permanent_redirect;
}
