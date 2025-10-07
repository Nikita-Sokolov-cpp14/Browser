#include "spider.h"
#include "../utils/secondary_function.h"

namespace {

const int maxRecursiveCount = 2;

} // namespace

Spider::Spider() :
dbmanager_(nullptr),
indexer_() {
}

void Spider::connectDb(DatabaseManager *dbManager) {
    dbmanager_ = dbManager;
    dbmanager_->clearDatabase();
}

void Spider::process() {
    while (!queueReferences_.empty()) {
        const QueueParams &queueParams = queueReferences_.front();
        std::cout << "host: " << queueParams.requestConfig.host
                  << " port: " << queueParams.requestConfig.port
                  << " target: " << queueParams.requestConfig.target << std::endl;
        std::string responseStr = client_.get(queueParams.requestConfig);
        // std::cout << responseStr << std::endl;
        indexer_.setPage(responseStr, queueParams.requestConfig.host, *dbmanager_);
        std::vector<RequestConfig> configs;
        extractAllLinks(responseStr, configs);
        for (auto config : configs) {
            if (queueParams.recursiveCount < maxRecursiveCount) {
                queueReferences_.push(QueueParams(config, queueParams.recursiveCount + 1));
            }
        }
        std::cout << "---" << std::endl;
        queueReferences_.pop();
    }
}

void Spider::start(const RequestConfig &startRequestConfig) {
    queueReferences_.push(QueueParams(startRequestConfig, 1));
    process();
}
