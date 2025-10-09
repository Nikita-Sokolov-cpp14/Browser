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
sslCtx_(ssl::context::tls_client) {
    // Более безопасная настройка SSL
    sslCtx_.set_default_verify_paths();
    sslCtx_.set_verify_mode(ssl::verify_peer); // Включаем проверку сертификата
    sslCtx_.set_options(ssl::context::default_workarounds | ssl::context::no_sslv2 |
            ssl::context::no_sslv3 | ssl::context::no_tlsv1 | ssl::context::single_dh_use);
}

SimpleHttpClient::~SimpleHttpClient() {
    // Безопасное закрытие соединений
    if (httpStream_ && httpStream_->socket().is_open()) {
        beast::error_code ec;
        httpStream_->socket().shutdown(tcp::socket::shutdown_both, ec);
        httpStream_->close();
    }

    if (httpsStream_ && beast::get_lowest_layer(*httpsStream_).socket().is_open()) {
        beast::error_code ec;
        beast::get_lowest_layer(*httpsStream_).close();
    }
}

std::string SimpleHttpClient::get(const RequestConfig &reqConfig, int max_redirects) {
    if (max_redirects <= 0) {
        throw std::runtime_error("Too many redirects");
    }

    RequestContext ctx {reqConfig, max_redirects};
    return performRequest(ctx);
}

std::string SimpleHttpClient::performRequest(const RequestContext &ctx) {
    if (ctx.config.port == "443") {
        return performHttpsRequest(ctx);
    } else {
        return performHttpRequest(ctx);
    }
}

std::string SimpleHttpClient::performHttpRequest(const RequestContext &ctx) {
    try {
        httpStream_ = std::make_unique<beast::tcp_stream>(ioc_);
        setupTimeouts(*httpStream_, ctx);

        std::cout << "Resolving: " << ctx.config.host << ":" << ctx.config.port << std::endl;
        auto const results = resolver_.resolve(ctx.config.host, ctx.config.port);

        std::cout << "Connecting to HTTP..." << std::endl;
        httpStream_->connect(results);

        http::request<http::string_body> req {http::verb::get, ctx.config.target, 11};
        req.set(http::field::host, ctx.config.host);
        req.set(http::field::user_agent, "Mozilla/5.0 (compatible; SimpleHttpClient)");
        req.set(http::field::accept, "*/*");
        req.set(http::field::accept_encoding, "identity"); // Отключаем сжатие для простоты

        std::cout << "Sending HTTP request to: " << ctx.config.host << ctx.config.target
                  << std::endl;
        http::write(*httpStream_, req);

        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;

        http::read(*httpStream_, buffer, res);

        std::string response = beast::buffers_to_string(res.body().data());
        std::cout << "Received HTTP response: " << res.result() << std::endl;

        if (is_redirect(res.result())) {
            auto location = res.find(http::field::location);
            if (location != res.end()) {
                std::string redirect_url = location->value().to_string();
                std::cout << "Redirecting to: " << redirect_url << std::endl;

                beast::error_code ec;
                httpStream_->socket().shutdown(tcp::socket::shutdown_both, ec);
                httpStream_.reset();

                return handleRedirect(redirect_url, ctx.maxRedirects - 1);
            }
        }

        beast::error_code ec;
        httpStream_->socket().shutdown(tcp::socket::shutdown_both, ec);
        httpStream_.reset();

        return response;

    } catch (const boost::system::system_error &e) {
        if (e.code() == boost::asio::error::operation_aborted ||
                e.code() == boost::asio::error::timed_out) {
            throw std::runtime_error("HTTP request timeout for " + ctx.config.host);
        }
        throw std::runtime_error("HTTP request failed for " + ctx.config.host + ": " + e.what());
    } catch (const std::exception &e) {
        if (httpStream_) {
            beast::error_code ec;
            httpStream_->socket().shutdown(tcp::socket::shutdown_both, ec);
            httpStream_.reset();
        }
        throw std::runtime_error("HTTP request failed for " + ctx.config.host + ": " + e.what());
    }
}

std::string SimpleHttpClient::performHttpsRequest(const RequestContext &ctx) {
    // TODO Хардкод, иначе метод handshake зависает.
    if (ctx.config.host == "play.google.com" ||
           ctx.config.host == "news.google.com" ||
            ctx.config.host == "www.youtube.com") {
        return "";
    }

    try {
        httpsStream_ = std::make_unique<beast::ssl_stream<beast::tcp_stream> >(ioc_, sslCtx_);

        setupTimeouts(beast::get_lowest_layer(*httpsStream_), ctx);

        // Установка SNI
        if (!SSL_set_tlsext_host_name(httpsStream_->native_handle(), ctx.config.host.c_str())) {
            beast::error_code ec {static_cast<int>(::ERR_get_error()),
                    net::error::get_ssl_category()};
            throw beast::system_error {ec};
        }

        std::cout << "Resolving: " << ctx.config.host << ":" << ctx.config.port << std::endl;
        auto const results = resolver_.resolve(ctx.config.host, ctx.config.port);

        std::cout << "Connecting to HTTPS..." << std::endl;
        beast::get_lowest_layer(*httpsStream_).connect(results);

        std::cout << "Performing SSL handshake..." << std::endl;
        httpsStream_->handshake(ssl::stream_base::client);
        std::cout << "SSL handshake successful" << std::endl;

        http::request<http::string_body> req {http::verb::get, ctx.config.target, 11};
        req.set(http::field::host, ctx.config.host);
        req.set(http::field::user_agent, "Mozilla/5.0 (compatible; SimpleHttpClient)");
        req.set(http::field::accept, "*/*");
        req.set(http::field::accept_encoding, "identity");

        std::cout << "Sending HTTPS request to: " << ctx.config.host << ctx.config.target
                  << std::endl;
        http::write(*httpsStream_, req);

        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;

        http::read(*httpsStream_, buffer, res);

        std::string response = beast::buffers_to_string(res.body().data());
        std::cout << "Received HTTPS response: " << res.result() << std::endl;

        if (is_redirect(res.result())) {
            auto location = res.find(http::field::location);
            if (location != res.end()) {
                std::string redirect_url = location->value().to_string();
                std::cout << "Redirecting to: " << redirect_url << std::endl;

                beast::error_code ec;
                httpsStream_->shutdown(ec);
                httpsStream_.reset();

                return handleRedirect(redirect_url, ctx.maxRedirects - 1);
            }
        }

        beast::error_code ec;
        httpsStream_->shutdown(ec);
        httpsStream_.reset();

        return response;

    } catch (const boost::system::system_error &e) {
        if (e.code() == boost::asio::error::operation_aborted ||
                e.code() == boost::asio::error::timed_out) {
            throw std::runtime_error("HTTPS request timeout for " + ctx.config.host);
        }
        throw std::runtime_error("HTTPS request failed for " + ctx.config.host + ": " + e.what());
    } catch (const std::exception &e) {
        if (httpsStream_) {
            beast::error_code ec;
            httpsStream_->shutdown(ec);
            httpsStream_.reset();
        }
        throw std::runtime_error("HTTPS request failed for " + ctx.config.host + ": " + e.what());
    }
}

std::string SimpleHttpClient::handleRedirect(const std::string &redirect_url, int max_redirects) {
    RequestConfig config;

    try {
        config = parseUrl(redirect_url);
    } catch (const std::exception &e) {
        throw std::runtime_error("Failed to parse redirect URL: " + std::string(e.what()));
    }

    RequestContext ctx {config, max_redirects};
    return performRequest(ctx);
}

bool SimpleHttpClient::is_redirect(http::status status) {
    return status == http::status::moved_permanently || status == http::status::found ||
            status == http::status::see_other || status == http::status::temporary_redirect ||
            status == http::status::permanent_redirect;
}
