#include <cstdlib>
#include <iostream>
#include <string>
#include <pqxx/pqxx>

#include "spider/spider.h"
#include "spider/indexer/indexer.h"
#include "database_manager/database_manager.h"

//! Тестовые данные для скачивания страницы. Это загружаемый аналог стартовой страницы.
namespace {

// http://www.google.ru/history/optout?hl=ru

const std::string host = "www.google.ru";
const std::string port = "80";
const std::string target = "/history/optout?hl=ru";

// const std::string host = "www.google.com";
// const std::string port = "80";
// const std::string target = "/";

} // namespace

int main(int argc, char **argv) {
    Spider spider;
    try {
        DatabaseManager dbmanager("host=localhost "
                                  "port=5432 "
                                  "dbname=browser_db "
                                  "user=nekit_pc "
                                  "password=987654321");

        RequestConfig reqConfig;
        reqConfig.host = host;
        reqConfig.port = port;
        reqConfig.target = target;

        spider.connectDb(&dbmanager);
        spider.start(reqConfig);
    } catch (const std::exception &e) {
        std::cout << e.what() << std::endl;
    }

    // const std::string url = "https://accounts.google.com:443/ServiceLogin?hl=ru&passive=true";
    // parseUrl(url);
}
