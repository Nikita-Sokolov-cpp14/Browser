#include "spider.h"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http; // from <boost/beast/http.hpp>
namespace net = boost::asio; // from <boost/asio.hpp>
using tcp = net::ip::tcp; // from <boost/asio/ip/tcp.hpp>

Spider::Spider() :
host(),
port(),
target(),
responseStr() {
}

Spider::Spider(const std::string &host, const std::string &port, const std::string &target) :
host(host),
port(port),
target(target),
responseStr() {
}

void Spider::loadPage() {
    try {
        // Check command line arguments.
        if (host == "" || port == "" || target == "") {
            std::cerr
                    << "Usage: http-client-sync <host> <port> <target> [<HTTP version: 1.0 or 1.1(default)>]\n"
                    << "Example:\n"
                    << "    http-client-sync www.example.com 80 /\n"
                    << "    http-client-sync www.example.com 80 / 1.0\n";
            return;
        }
        // int version = argc == 5 && !std::strcmp("1.0", argv[4]) ? 10 : 11;
        //! TODO: Исправить
        int version = 10;

        // The io_context is required for all I/O
        net::io_context ioc;

        // These objects perform our I/O
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);

        // Look up the domain name
        auto const results = resolver.resolve(host, port);

        // Make the connection on the IP address we get from a lookup
        stream.connect(results);

        // Set up an HTTP GET request message
        http::request<http::string_body> req {http::verb::get, target, version};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // Send the HTTP request to the remote host
        http::write(stream, req);

        // This buffer is used for reading and must be persisted
        beast::flat_buffer buffer;

        // Declare a container to hold the response
        http::response<http::dynamic_body> res;

        // Receive the HTTP response
        http::read(stream, buffer, res);

        responseStr = boost::beast::buffers_to_string(res.body().data());

        // Gracefully close the socket
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        // not_connected happens sometimes
        // so don't bother reporting it.
        //
        if (ec && ec != beast::errc::not_connected)
            throw beast::system_error {ec};

        // If we get here then the connection is closed gracefully
    } catch (std::exception const &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return;
    }
    return;
}

std::string& Spider::getResponseStr() {
    return responseStr;
}
