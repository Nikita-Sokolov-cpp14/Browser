#pragma once

#include <cstdlib>
#include <iostream>
#include <string>
#include <queue>

#include "indexer/indexer.h"
#include "../database_manager/database_manager.h"
#include "../common_data.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <boost/beast/ssl.hpp>
#include <boost/asio/ssl.hpp>

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http; // from <boost/beast/http.hpp>
namespace net = boost::asio; // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp; // from <boost/asio/ip/tcp.hpp>

struct QueueParams {
    RequestConfig requestConfig;
    size_t recursiveCount;

    QueueParams(const RequestConfig &reqConfig, size_t recursiveCnt) {
        requestConfig = reqConfig;
        recursiveCount = recursiveCnt;
    }
};

class SimpleHttpClient {
public:
    SimpleHttpClient();

    std::string get(const RequestConfig &reqConfig, int max_redirects);

private:
    bool is_redirect(http::status status);

    net::io_context ioc_;
    tcp::resolver resolver_;
    beast::tcp_stream stream_;
};

/**
* @brief Класс программы «Паук».
* @details HTTP-клиент, задача которого — парсить сайты и строить индексы,
* исходя из частоты слов в документах.
*/
class Spider {
public:
    Spider();

    void start(const RequestConfig &startRequestConfig);

    void connectDb(DatabaseManager *dbManager);

private:
    // std::string responseStr_; //!< Строка с ответом на запрос.
    std::queue<QueueParams> queueReferences_;
    DatabaseManager *dbmanager_;
    Indexer indexer_;
    SimpleHttpClient client_;

    void process();
};
