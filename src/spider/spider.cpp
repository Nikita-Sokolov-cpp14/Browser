#include "spider.h"
#include "../utils/secondary_function.h"

Spider::Spider() :
dbmanager_(nullptr),
stop_(false),
maxRecursiveCount_(1) {
    // Создаем пул потоков по умолчанию (количество = hardware_concurrency)
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

void Spider::connectDb(DatabaseManager *dbManager) {
    dbmanager_ = dbManager;
    dbmanager_->clearDatabase();
}

void Spider::setThreadCount(size_t count) {
    // Останавливаем старые потоки если есть
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

    // Запускаем новые потоки
    stop_ = false;
    for (size_t i = 0; i < count; ++i) {
        workers_.emplace_back(&Spider::workerThread, this);
    }
}

void Spider::addTask(const QueueParams& task) {
    std::unique_lock<std::mutex> lock(queueMutex_);
    tasksQueue_.push(task);
    condition_.notify_one();
}

void Spider::workerThread() {
    while (true) {
        QueueParams task;

        {
            std::unique_lock<std::mutex> lock(queueMutex_);

            // Ждем либо новые задачи, либо сигнал остановки
            condition_.wait(lock, [this]() {
                return stop_ || !tasksQueue_.empty();
            });

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

        // Обрабатываем задачу (без блокировки мьютекса)
        processTask(task);
        activeTasks_--;
    }
}

void Spider::processTask(const QueueParams &queueParams) {
    std::cout << "[" << std::this_thread::get_id() << "] "
              << "host: " << queueParams.requestConfig.host
              << " port: " << queueParams.requestConfig.port
              << " target: " << queueParams.requestConfig.target << std::endl;

    auto client = std::make_unique<SimpleHttpClient>();
    std::string responseStr = client->get(queueParams.requestConfig);

    // Вычисления индексации без блокировки
    auto indexer = std::make_unique<Indexer>();
    indexer->setPage(responseStr);

    // Короткая блокировка только для записи в БД
    {
        std::unique_lock<std::mutex> dbLock(dbMutex_);
        indexer->saveDataToDb(*dbmanager_, queueParams.requestConfig.host);
    }

    // Извлекаем ссылки
    std::vector<RequestConfig> configs;
    {
        std::unique_lock<std::mutex> xmlLock(xmlMutex_);
        extractAllLinks(responseStr, configs);
    }

    // Добавляем новые задачи в очередь
    for (auto& config : configs) {
        if (queueParams.recursiveCount < maxRecursiveCount_) {
            addTask(QueueParams(config, queueParams.recursiveCount + 1));
        }
    }

    std::cout << "[" << std::this_thread::get_id() << "] ---" << std::endl;
}

void Spider::start(const RequestConfig &startRequestConfig, int recursiveCount) {
    maxRecursiveCount_ = recursiveCount;
    // Добавляем начальную задачу
    addTask(QueueParams(startRequestConfig, 1));

    // Ждем завершения всех задач
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        std::unique_lock<std::mutex> lock(queueMutex_);
        if (tasksQueue_.empty() && activeTasks_ == 0) {
            break;
        }
    }

    // Останавливаем потоки
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        stop_ = true;
    }
    condition_.notify_all();
}
