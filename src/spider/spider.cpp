#include "spider.h"
#include "../utils/secondary_function.h"

Spider::Spider() :
dbmanager_(nullptr),
stop_(false),
maxRecursiveCount_(1) {
    setThreadCount(std::thread::hardware_concurrency());
    // setThreadCount(1);
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

void Spider::setDbManager(DatabaseManager *dbManager) {
    dbmanager_ = dbManager;
    dbmanager_->clearDatabase();
}

void Spider::setThreadCount(size_t count) {
    // Остановить старые потоки если есть
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

    workers_.clear();

    // Запустить новые потоки
    stop_ = false;
    for (size_t i = 0; i < count; ++i) {
        workers_.emplace_back(&Spider::workerThread, this);
    }
}

void Spider::addTask(const QueueParams &task) {
    std::unique_lock<std::mutex> lock(queueMutex_);
    tasksQueue_.push(task);
    condition_.notify_one();
}

void Spider::workerThread() {
    while (true) {
        QueueParams task;

        {
            std::unique_lock<std::mutex> lock(queueMutex_);

            // Ожидание либо новых задач, либо сигнала остановки
            condition_.wait(lock, [this]() { return stop_ || !tasksQueue_.empty(); });

            // Если остановка и очередь пуста - выходим
            if (stop_ && tasksQueue_.empty()) {
                return;
            }

            // Если есть задачи - берем следующую
            if (!tasksQueue_.empty()) {
                task = std::move(tasksQueue_.front());
                tasksQueue_.pop();
                activeTasks_++;
            } else {
                continue;
            }
        }

        // Обрабатываем задачу
        processTask(task);
        activeTasks_--;

        if (tasksQueue_.empty() && activeTasks_ == 0) {
            condition_.notify_all();
        }
    }
}

void Spider::processTask(const QueueParams &queueParams) {
    try {
        std::cout << "Spider::processTask: start task: "
                  << "host: " << queueParams.requestConfig.host
                  << " port: " << queueParams.requestConfig.port
                  << " target: " << queueParams.requestConfig.target << std::endl;

        auto client = std::make_unique<PageLoader>();
        std::string responseStr = client->get(queueParams.requestConfig);

        std::cout << "processTask: get responseStr" << std::endl;
        auto indexer = std::make_unique<Indexer>();
        indexer->setPage(responseStr);

        {
            std::unique_lock<std::mutex> dbLock(dbMutex_);
            indexer->saveDataToDb(*dbmanager_, queueParams.requestConfig);
            std::cout << "processTask: stop saveDataToDb" << std::endl;
        }

        // Извлекаем ссылки
        // std::vector<RequestConfig> configs;
        // {
        //     std::unique_lock<std::mutex> xmlLock(xmlMutex_);
        //     extractAllLinks(responseStr, configs);
        // }
        std::vector<RequestConfig> configs;
        extractAllLinks(responseStr, configs);

        // Добавляем новые задачи в очередь
        for (auto &config : configs) {
            if (queueParams.recursiveCount < maxRecursiveCount_) {
                addTask(QueueParams(config, queueParams.recursiveCount + 1));
            }
        }
    } catch (std::exception &err) {
        std::cerr << "Spider::processTask: ERROR" << err.what() << std::endl;
    }
}

void Spider::start(const RequestConfig &startRequestConfig, int recursiveCount) {
    maxRecursiveCount_ = recursiveCount;
    addTask(QueueParams(startRequestConfig, 1));

    // Эффективное ожидание вместо sleep
    std::unique_lock<std::mutex> lock(queueMutex_);
    condition_.wait(lock, [this]() { return tasksQueue_.empty() && activeTasks_ == 0; });

    stop_ = true;
    condition_.notify_all();
}
