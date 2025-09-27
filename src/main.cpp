#include <cstdlib>
#include <iostream>
#include <string>
#include <pqxx/pqxx>

#include "spider/spider.h"
#include "spider/indexer/indexer.h"
#include "database_manager/database_manager.h"

//! Тестовые данные для скачивания страницы. Это загружаемый аналог стартовой страницы.
namespace {

const std::string host = "www.google.com";
const std::string port = "80";
const std::string target = "/";

} // namespace

int main(int argc, char **argv) {
    Spider spider;
    spider.loadPage(host, port, target);

    //! TODO: Индексатор пока отдельно, но в будущем будет внутри Spider.
    Indexer indexer;

    // for (const auto &val : indexer.getStorage()) {
    //     std::cout << val.first << " " << val.second << std::endl;
    // }

    try {
        DatabaseManager dbmanager("host=localhost "
                           "port=5432 "
                           "dbname=browser_db "
                           "user=nekit_pc "
                           "password=987654321");

        dbmanager.clearDatabase();
        indexer.setPage(spider.getResponseStr(), host, dbmanager);
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
    }

    // Проверка резултатов. Должа вывестись строка без HTML тегов, без знаков препинания и в нижнем регистре
    // std::cout << indexer.getText() << std::endl;
}
