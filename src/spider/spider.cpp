#include "spider.h"
#include "../utils/secondary_function.h"

namespace {

const int maxRecursiveCount = 3;

} // namespace

Spider::Spider() :
dbmanager_(nullptr),
stop_(false) {
    // Создаем пул потоков по умолчанию (количество = hardware_concurrency)
    setThreadCount(std::thread::hardware_concurrency());
}

Spider::~Spider() {
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        stop_ = true;
    }
    condition_.notify_all();

    for (std::thread &worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void Spider::connectDb(DatabaseManager *dbManager) {
    dbmanager_ = dbManager;
    dbmanager_->clearDatabase();
}

void Spider::setThreadCount(size_t count) {
    // Запускаем новые потоки
    stop_ = false;
    for (size_t i = 0; i < count; ++i) {
        workers_.emplace_back(&Spider::workerThread, this);
    }
}

void Spider::workerThread() {
    while (true) {
        std::pair<QueueParams, std::promise<void> > task;

        {
            std::unique_lock<std::mutex> lock(queueMutex_);

            condition_.wait(lock, [this]() { return stop_ || !tasks_.empty(); });

            if (stop_ && tasksQueue_.empty()) {
                return;
            }

            task = std::move(tasksQueue_.front());
            tasksQueue_.pop();
        }

        processTask(task.first);
        task.second.set_value();

        // Уведомляем основной поток, что задача завершена
        condition_.notify_one();
    }
}

void Spider::processTask(const QueueParams &queueParams) {
    std::cout << "[" << std::this_thread::get_id() << "] "
              << "host: " << queueParams.requestConfig.host
              << " port: " << queueParams.requestConfig.port
              << " target: " << queueParams.requestConfig.target << std::endl;

    SimpleHttpClient client;
    std::string responseStr = client.get(queueParams.requestConfig);
    Indexer indexer;
    indexer.setPage(responseStr);
    // Блокируем доступ к БД для индексации
    {
        std::unique_lock<std::mutex> dbLock(dbMutex_);
        indexer.saveDataToDb(*dbmanager_, queueParams.requestConfig.host);
    }

    std::vector<RequestConfig> configs;
    extractAllLinks(responseStr, configs);

    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        for (auto &config : configs) {
            if (queueParams.recursiveCount < maxRecursiveCount) {
                tasksQueue_.push(QueueParams(config, queueParams.recursiveCount + 1));
            }
        }
    }
    condition_.notify_all(); // Уведомляем все потоки о новых задачах

    std::cout << "[" << std::this_thread::get_id() << "] ---" << std::endl;
}

void Spider::process() {
    // Основной процесс теперь управляет очередью, а потоки обрабатывают задачи
    while (true) {
        QueueParams task;

        {
            std::unique_lock<std::mutex> lock(queueMutex_);

            // Ждем новые задачи или завершение
            condition_.wait(lock, [this]() { return stop_ || !tasksQueue_.empty(); });

            if (stop_ && tasksQueue_.empty())
                return;
            if (tasksQueue_.empty())
                continue;

            task = std::move(tasksQueue_.front());
            tasksQueue_.pop();
        }

        processTask(task);
    }
}

void Spider::start(const RequestConfig &startRequestConfig) {
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        tasksQueue_.push(QueueParams(startRequestConfig, 1));
    }
    condition_.notify_all();

    process(); // Теперь process() работает в основном потоке параллельно с workerThreads
}
