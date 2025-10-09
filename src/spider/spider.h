#pragma once

#include <cstdlib>
#include <iostream>
#include <string>
#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <future>

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

    // Добавляем конструктор по умолчанию
    QueueParams() = default;
};

/**
* @brief Класс программы «Паук».
* @details HTTP-клиент, задача которого — парсить сайты и строить индексы,
* исходя из частоты слов в документах.
*/
class Spider {
public:
    Spider();
    ~Spider();

    void start(const RequestConfig &startRequestConfig, int recursiveCount);

    void connectDb(DatabaseManager *dbManager);

    void setThreadCount(size_t count); // Установка количества потоков

private:
    DatabaseManager *dbmanager_;

    // Единая очередь задач
    std::queue<QueueParams> tasksQueue_;
    std::vector<std::thread> workers_;
    std::mutex queueMutex_;
    std::mutex dbMutex_; // Мьютекс для БД
    std::condition_variable condition_;
    std::atomic<bool> stop_;
    std::atomic<size_t> activeTasks_{0}; // Счетчик активных задач

    std::mutex xmlMutex_; // Добавляем этот мьютекс

    int maxRecursiveCount_;

    void workerThread();
    void processTask(const QueueParams &queueParams);
    void addTask(const QueueParams& task); // Добавление задачи в очередь
};
