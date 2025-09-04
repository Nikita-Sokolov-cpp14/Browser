#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

class Spider {
public:
    Spider();

    Spider(const std::string &host, const std::string &port, const std::string &target);

    void loadPage();

    std::string& getResponseStr();

private:
    std::string host;
    std::string port;
    std::string target;
    std::string responseStr;
};
