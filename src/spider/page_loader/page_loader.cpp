#include "page_loader.h"
#include "../utils/secondary_function.h"

#include <iostream>
#include <chrono>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp;

SimpleHttpClient::SimpleHttpClient() :
resolver_(ioc_),
ssl_ctx_(ssl::context::tls_client) {
    // Более безопасная настройка SSL
    ssl_ctx_.set_default_verify_paths();
    ssl_ctx_.set_verify_mode(ssl::verify_peer); // Включаем проверку сертификата
    ssl_ctx_.set_options(ssl::context::default_workarounds | ssl::context::no_sslv2 |
            ssl::context::no_sslv3 | ssl::context::no_tlsv1 | ssl::context::single_dh_use);
}

SimpleHttpClient::~SimpleHttpClient() {
    // Безопасное закрытие соединений
    if (http_stream_ && http_stream_->socket().is_open()) {
        beast::error_code ec;
        http_stream_->socket().shutdown(tcp::socket::shutdown_both, ec);
        http_stream_->close();
    }

    if (https_stream_ && beast::get_lowest_layer(*https_stream_).socket().is_open()) {
        beast::error_code ec;
        beast::get_lowest_layer(*https_stream_).close();
    }
}

template<typename Stream>
void SimpleHttpClient::setup_timeouts(Stream &stream, const RequestContext &ctx) {
    stream.expires_after(ctx.timeout);
}

RequestConfig SimpleHttpClient::parse_url_safe(const std::string &url) {
    try {
        return parseUrl(url);
    } catch (const std::exception &e) {
        throw std::runtime_error("Failed to parse redirect URL: " + std::string(e.what()));
    }
}

std::string SimpleHttpClient::get(const RequestConfig &reqConfig, int max_redirects) {
    if (max_redirects <= 0) {
        throw std::runtime_error("Too many redirects");
    }

    RequestContext ctx {reqConfig, max_redirects};
    return perform_request(ctx);
}

std::string SimpleHttpClient::perform_request(const RequestContext &ctx) {
    if (ctx.config.port == "443") {
        return perform_https_request(ctx);
    } else {
        return perform_http_request(ctx);
    }
}

std::string SimpleHttpClient::perform_http_request(const RequestContext &ctx) {
    try {
        http_stream_ = std::make_unique<beast::tcp_stream>(ioc_);
        setup_timeouts(*http_stream_, ctx);

        std::cout << "Resolving: " << ctx.config.host << ":" << ctx.config.port << std::endl;
        auto const results = resolver_.resolve(ctx.config.host, ctx.config.port);

        std::cout << "Connecting to HTTP..." << std::endl;
        http_stream_->connect(results);

        http::request<http::string_body> req {http::verb::get, ctx.config.target, 11};
        req.set(http::field::host, ctx.config.host);
        req.set(http::field::user_agent, "Mozilla/5.0 (compatible; SimpleHttpClient)");
        req.set(http::field::accept, "*/*");
        req.set(http::field::accept_encoding, "identity"); // Отключаем сжатие для простоты

        std::cout << "Sending HTTP request to: " << ctx.config.host << ctx.config.target
                  << std::endl;
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
                std::cout << "Redirecting to: " << redirect_url << std::endl;

                beast::error_code ec;
                http_stream_->socket().shutdown(tcp::socket::shutdown_both, ec);
                http_stream_.reset();

                return handle_redirect(redirect_url, ctx.max_redirects - 1);
            }
        }

        beast::error_code ec;
        http_stream_->socket().shutdown(tcp::socket::shutdown_both, ec);
        http_stream_.reset();

        return response;

    } catch (const boost::system::system_error &e) {
        if (e.code() == boost::asio::error::operation_aborted ||
                e.code() == boost::asio::error::timed_out) {
            throw std::runtime_error("HTTP request timeout for " + ctx.config.host);
        }
        throw std::runtime_error("HTTP request failed for " + ctx.config.host + ": " + e.what());
    } catch (const std::exception &e) {
        if (http_stream_) {
            beast::error_code ec;
            http_stream_->socket().shutdown(tcp::socket::shutdown_both, ec);
            http_stream_.reset();
        }
        throw std::runtime_error("HTTP request failed for " + ctx.config.host + ": " + e.what());
    }
}

std::string SimpleHttpClient::perform_https_request(const RequestContext &ctx) {
    // TODO Хардкод, иначе метод handshake зависает.
    if (ctx.config.host == "play.google.com" ||
           ctx.config.host == "news.google.com") {
        return "";
    }

    try {
        https_stream_ = std::make_unique<beast::ssl_stream<beast::tcp_stream> >(ioc_, ssl_ctx_);

        setup_timeouts(beast::get_lowest_layer(*https_stream_), ctx);

        // Установка SNI
        if (!SSL_set_tlsext_host_name(https_stream_->native_handle(), ctx.config.host.c_str())) {
            beast::error_code ec {static_cast<int>(::ERR_get_error()),
                    net::error::get_ssl_category()};
            throw beast::system_error {ec};
        }

        std::cout << "Resolving: " << ctx.config.host << ":" << ctx.config.port << std::endl;
        auto const results = resolver_.resolve(ctx.config.host, ctx.config.port);

        std::cout << "Connecting to HTTPS..." << std::endl;
        beast::get_lowest_layer(*https_stream_).connect(results);

        std::cout << "Performing SSL handshake..." << std::endl;
        https_stream_->handshake(ssl::stream_base::client);
        std::cout << "SSL handshake successful" << std::endl;

        http::request<http::string_body> req {http::verb::get, ctx.config.target, 11};
        req.set(http::field::host, ctx.config.host);
        req.set(http::field::user_agent, "Mozilla/5.0 (compatible; SimpleHttpClient)");
        req.set(http::field::accept, "*/*");
        req.set(http::field::accept_encoding, "identity");

        std::cout << "Sending HTTPS request to: " << ctx.config.host << ctx.config.target
                  << std::endl;
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
                std::cout << "Redirecting to: " << redirect_url << std::endl;

                beast::error_code ec;
                https_stream_->shutdown(ec);
                https_stream_.reset();

                return handle_redirect(redirect_url, ctx.max_redirects - 1);
            }
        }

        beast::error_code ec;
        https_stream_->shutdown(ec);
        https_stream_.reset();

        return response;

    } catch (const boost::system::system_error &e) {
        if (e.code() == boost::asio::error::operation_aborted ||
                e.code() == boost::asio::error::timed_out) {
            throw std::runtime_error("HTTPS request timeout for " + ctx.config.host);
        }
        throw std::runtime_error("HTTPS request failed for " + ctx.config.host + ": " + e.what());
    } catch (const std::exception &e) {
        if (https_stream_) {
            beast::error_code ec;
            https_stream_->shutdown(ec);
            https_stream_.reset();
        }
        throw std::runtime_error("HTTPS request failed for " + ctx.config.host + ": " + e.what());
    }
}

std::string SimpleHttpClient::handle_redirect(const std::string &redirect_url, int max_redirects) {
    RequestConfig config = parse_url_safe(redirect_url);
    RequestContext ctx {config, max_redirects};
    return perform_request(ctx);
}

bool SimpleHttpClient::is_redirect(http::status status) {
    return status == http::status::moved_permanently || status == http::status::found ||
            status == http::status::see_other || status == http::status::temporary_redirect ||
            status == http::status::permanent_redirect;
}
