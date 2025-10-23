#include "page_loader.h"
#include "../utils/secondary_function.h"

#include <iostream>
#include <chrono>
#include <vector>

namespace {} // namespace

PageLoader::PageLoader() :
resolver_(ioc_),
sslCtx_(ssl::context::tls_client) {
    sslCtx_.set_default_verify_paths();
    sslCtx_.set_verify_mode(ssl::verify_peer);
    sslCtx_.set_options(ssl::context::default_workarounds | ssl::context::no_sslv2 |
            ssl::context::no_sslv3 | ssl::context::no_tlsv1 | ssl::context::single_dh_use);
}

PageLoader::~PageLoader() {
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

std::string PageLoader::get(const RequestConfig &reqConfig, int countRedirects) {
    if (countRedirects <= 0) {
        throw std::runtime_error("Too many redirects");
    }

    RequestContext ctx {reqConfig, countRedirects};
    return performRequest(ctx);
}

std::string PageLoader::performRequest(const RequestContext &ctx) {
    if (ctx.config.port == "443") {
        return performHttpsRequest(ctx);
    } else {
        return performHttpRequest(ctx);
    }
}

std::string PageLoader::performHttpRequest(const RequestContext &ctx) {
    try {
        httpStream_ = std::make_unique<beast::tcp_stream>(ioc_);

        // Асинхронный connect с таймаутом
        beast::error_code connect_ec;
        bool connect_completed = false;

        auto const results = resolver_.resolve(ctx.config.host, ctx.config.port);

        // Таймер для connect
        net::steady_timer connect_timer(ioc_);
        connect_timer.expires_after(ctx.timeout);

        // Асинхронный connect
        httpStream_->async_connect(results,
                [&connect_ec, &connect_completed](beast::error_code ec,
                        tcp::resolver::results_type::endpoint_type) {
                    connect_ec = ec;
                    connect_completed = true;
                });

        // Асинхронное ожидание таймаута
        connect_timer.async_wait([&connect_completed, this](beast::error_code ec) {
            if (!ec && !connect_completed) {
                // Таймаут сработал до завершения connect
                std::cout << "HTTP connect timeout, cancelling..." << std::endl;
                httpStream_->cancel();
            }
        });

        // Запускаем io_context до завершения connect или таймаута
        ioc_.restart();
        while (ioc_.run_one()) {
            if (connect_completed) {
                connect_timer.cancel();
                break;
            }
        }

        // Проверяем результат connect
        if (connect_ec) {
            if (connect_ec == net::error::operation_aborted) {
                throw std::runtime_error("HTTP connect timeout for " + ctx.config.host);
            }
            throw beast::system_error(connect_ec);
        }

        std::cout << "HTTP connect completed successfully" << std::endl;
        std::cout << "performHttpRequest connect" << std::endl;

        // Устанавливаем таймаут для последующих операций
        setupTimeouts(*httpStream_, ctx);

        http::request<http::string_body> req {http::verb::get, ctx.config.target, 11};
        req.set(http::field::host, ctx.config.host);
        req.set(http::field::user_agent, "Mozilla/5.0 (compatible; PageLoader)");
        req.set(http::field::accept, "*/*");
        req.set(http::field::accept_encoding, "identity"); // Отключаем сжатие для простоты
        http::write(*httpStream_, req);

        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;

        http::read(*httpStream_, buffer, res);

        std::string response = beast::buffers_to_string(res.body().data());
        std::cout << "Received HTTP response for " << ctx.config.host << " " << ctx.config.port
                  << " " << ctx.config.target << " " << res.result() << std::endl;

        if (isRedirect(res.result())) {
            auto location = res.find(http::field::location);
            if (location != res.end()) {
                std::string redirect_url = location->value().to_string();
                std::cout << "Redirecting to: " << redirect_url << std::endl;

                beast::error_code ec;
                httpStream_->socket().shutdown(tcp::socket::shutdown_both, ec);
                httpStream_.reset();

                return handleRedirect(redirect_url, ctx.countRedirects - 1, ctx.config);
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

std::string PageLoader::performHttpsRequest(const RequestContext &ctx) {
    try {
        httpsStream_ = std::make_unique<beast::ssl_stream<beast::tcp_stream> >(ioc_, sslCtx_);

        setupTimeouts(beast::get_lowest_layer(*httpsStream_), ctx);

        // Установка SNI
        if (!SSL_set_tlsext_host_name(httpsStream_->native_handle(), ctx.config.host.c_str())) {
            beast::error_code ec {static_cast<int>(::ERR_get_error()),
                    net::error::get_ssl_category()};
            throw beast::system_error {ec};
        }

        auto const results = resolver_.resolve(ctx.config.host, ctx.config.port);
        beast::get_lowest_layer(*httpsStream_).connect(results);

        // Асинхронный handshake с таймаутом
        beast::error_code handshake_ec;
        bool handshake_completed = false;

        // Таймер для handshake
        net::steady_timer handshake_timer(ioc_);
        handshake_timer.expires_after(ctx.timeout);

        // Асинхронный handshake
        httpsStream_->async_handshake(ssl::stream_base::client,
                [&handshake_ec, &handshake_completed](beast::error_code ec) {
                    handshake_ec = ec;
                    handshake_completed = true;
                });

        // Асинхронное ожидание таймаута
        handshake_timer.async_wait([&handshake_completed, this](beast::error_code ec) {
            if (!ec && !handshake_completed) {
                // Таймаут сработал до завершения handshake
                std::cout << "SSL handshake timeout, cancelling..." << std::endl;
                beast::get_lowest_layer(*httpsStream_).cancel();
            }
        });

        // Запускаем io_context до завершения handshake или таймаута
        ioc_.restart();
        while (ioc_.run_one()) {
            if (handshake_completed) {
                handshake_timer.cancel();
                break;
            }
        }

        // Проверяем результат handshake
        if (handshake_ec) {
            if (handshake_ec == net::error::operation_aborted) {
                throw std::runtime_error("SSL handshake timeout for " + ctx.config.host);
            }
            throw beast::system_error(handshake_ec);
        }

        std::cout << "SSL handshake completed successfully" << std::endl;
        std::cout << "performHttpsRequest" << std::endl;

        // Остальной код остается без изменений
        http::request<http::string_body> req {http::verb::get, ctx.config.target, 11};
        req.set(http::field::host, ctx.config.host);
        req.set(http::field::user_agent, "Mozilla/5.0 (compatible; PageLoader)");
        req.set(http::field::accept, "*/*");
        req.set(http::field::accept_encoding, "identity");

        http::write(*httpsStream_, req);

        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;

        http::read(*httpsStream_, buffer, res);

        std::string response = beast::buffers_to_string(res.body().data());
        std::cout << "Received HTTP response for " << ctx.config.host << " " << ctx.config.port
                  << " " << ctx.config.target << " " << res.result() << std::endl;

        if (isRedirect(res.result())) {
            auto location = res.find(http::field::location);
            if (location != res.end()) {
                std::string redirect_url = location->value().to_string();
                std::cout << "Redirecting to: " << redirect_url << std::endl;

                beast::error_code ec;
                httpsStream_->shutdown(ec);
                httpsStream_.reset();

                return handleRedirect(redirect_url, ctx.countRedirects - 1, ctx.config);
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

std::string PageLoader::handleRedirect(const std::string &redirect_url, int countRedirects,
        const RequestConfig &sourceConfig) {
    RequestConfig config;
    try {
        config = parseUrl(redirect_url, sourceConfig);
    } catch (const std::exception &e) {
        throw std::runtime_error("Failed to parse redirect URL: " + std::string(e.what()));
    }

    RequestContext ctx {config, countRedirects};
    return performRequest(ctx);
}

bool PageLoader::isRedirect(http::status status) {
    return status == http::status::moved_permanently || status == http::status::found ||
            status == http::status::see_other || status == http::status::temporary_redirect ||
            status == http::status::permanent_redirect;
}
