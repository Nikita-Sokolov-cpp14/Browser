#pragma once

#include <cstdlib>
#include <iostream>
#include <string>
#include <queue>

#include "indexer/indexer.h"
#include "../database_manager/database_manager.h"

struct RequestConfig {
    std::string host;
    std::string port;
    std::string target;
};

struct QueueParams {
    RequestConfig requestConfig;
    size_t recursiveCount;

    QueueParams(const RequestConfig &reqConfig, size_t recursiveCnt) {
        requestConfig = reqConfig;
        recursiveCount = recursiveCnt;
    }
};

RequestConfig parseUrl(const std::string &url);

std::string getPage(const RequestConfig &startRequestConfig, int maxRedirects);

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

    void process();
};
