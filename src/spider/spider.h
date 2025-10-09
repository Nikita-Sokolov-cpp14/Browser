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

/**
* @brief Структура для выполнения задачи скачивания HTML страниц.
*/
struct QueueParams {
    RequestConfig requestConfig; //!< Параметры подключения к HTML странице.
    size_t recursiveCount; //!< Текущая глубина рекурсии.

    /**
    * @brief Конструктор.
    * @param reqConfig Параметры подключения к HTML странице.
    * @param recursiveCnt //!< Текущая глубина рекурсии.
    */
    QueueParams(const RequestConfig &reqConfig, size_t recursiveCnt) {
        requestConfig = reqConfig;
        recursiveCount = recursiveCnt;
    }

    /**
    * @brief Конструктор по умолчанию.
    */
    QueueParams() = default;
};

/**
* @brief Класс программы «Паук».
* @details Парсит сайты и строить индексы, исходя из частоты слов в документах.
*/
class Spider {
public:
    /**
    * @brief Конструктор.
    */
    Spider();

    /**
    * @brief Деструктор.
    */
    ~Spider();

    /**
    * @brief Запустить стартовую задачу.
    * @param startRequestConfig Параметры подключения к HTML странице.
    * @param recursiveCount //!< Максимальная глубина рекурсии.
    */
    void start(const RequestConfig &startRequestConfig, int recursiveCount);

    /**
    * @brief Установить объект взаимодействия с БД.
    */
    void setDbManager(DatabaseManager *dbManager);

    /**
    * @brief Установить число потоков.
    */
    void setThreadCount(size_t count);

private:
    DatabaseManager *dbmanager_; //!< Объект взаимодействия с БД PostgreSql.
    std::queue<QueueParams> tasksQueue_; //!< Очередь задач.
    std::vector<std::thread> workers_; //!< Контейнер рабочих потоков.
    std::mutex queueMutex_; //!< Мьютекс для работы с очередью.
    std::mutex dbMutex_; //!< Мьютекс для работы с БД.
    std::condition_variable condition_;
    std::atomic<bool> stop_; //!< Условие остановки.
    std::atomic<size_t> activeTasks_ {0}; //!< Счетчик активных задач.
    // std::mutex xmlMutex_;
    int maxRecursiveCount_; //!< Максимальная глубина рекурсии.

    /**
    * @brief Обработать состояние потоков.
    */
    void workerThread();

    /**
    * @brief Запустить задачу скачивания и индексации HTML страницы.
    */
    void processTask(const QueueParams &queueParams);

    /**
    * @brief Добавить задачу скачивания и индексации HTML страницы в очередь.
    */
    void addTask(const QueueParams &task);
};
