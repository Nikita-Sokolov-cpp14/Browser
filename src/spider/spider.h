#pragma once

#include <cstdlib>
#include <iostream>
#include <string>
#include <queue>

#include "indexer/indexer.h"
#include "../database_manager/database_manager.h"
#include "../common_data.h"
#include "page_loader/page_loader.h"

struct QueueParams {
    RequestConfig requestConfig;
    size_t recursiveCount;

    QueueParams(const RequestConfig &reqConfig, size_t recursiveCnt) {
        requestConfig = reqConfig;
        recursiveCount = recursiveCnt;
    }
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
