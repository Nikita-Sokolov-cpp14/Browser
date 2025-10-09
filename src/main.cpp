#include <cstdlib>
#include <iostream>
#include <string>
#include <pqxx/pqxx>

#include "spider/spider.h"
#include "spider/indexer/indexer.h"
#include "database_manager/database_manager.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

//! Тестовые данные для скачивания страницы. Это загружаемый аналог стартовой страницы.
namespace {

// http://www.google.ru/history/optout?hl=ru

// const std::string host = "www.google.ru";
// const std::string port = "80";
// const std::string target = "/history/optout?hl=ru";

// const std::string host = "www.ya.ru";
// const std::string port = "443";
// const std::string target = "/";

// const std::string host = "www.google.com";
// const std::string port = "80";
// const std::string target = "/";

// const std::string host = "play.google.com";
// const std::string port = "443";
// const std::string target = "/?hl=ru&tab=w8";

} // namespace

/**
* @brief Структура параметров подключения к БД PostgreSql.
*/
struct DatabaseConfig {
    std::string host;
    std::string port;
    std::string dbname;
    std::string user;
    std::string password;
};

/**
* @brief Структура параметров подключения к стартовой HTML странице.
*/
struct StartPageParams {
    std::string host;
    std::string port;
    std::string target;
};

/**
* @brief Стартовая структура.
*/
struct StartConfig {
    std::string dbConnectionString;
    StartPageParams startPageParams;
    int recursiveCount; //! Глубина рекурсии.
};

/**
* @brief Преобразовать структуру подключения к БД в строку.
* @param dbConfig Структура подключения к БД.
* @return Строка спараметрами подключения к БД.
*/
std::string getConnectionString(const DatabaseConfig &dbConfig) {
    std::string dBconnectionString;
    dBconnectionString = "host=" + dbConfig.host;
    dBconnectionString += " port=" + dbConfig.port;
    dBconnectionString += " dbname=" + dbConfig.dbname;
    dBconnectionString += " user=" + dbConfig.user;
    dBconnectionString += " password=" + dbConfig.password;

    return dBconnectionString;
}

/**
* @brief Загрузить данные из файла конфигурации.
* @param startConfig Структура для записи.
*/
void readConfig(StartConfig &startConfig) {
    boost::property_tree::ptree pt;

    try {
        boost::property_tree::read_ini("../resources/config.ini", pt);

        DatabaseConfig dbConfig;
        dbConfig.host = pt.get<std::string>("Database.host");
        dbConfig.port = pt.get<std::string>("Database.port");
        dbConfig.dbname = pt.get<std::string>("Database.dbname");
        dbConfig.user = pt.get<std::string>("Database.user");
        dbConfig.password = pt.get<std::string>("Database.password");
        startConfig.dbConnectionString = getConnectionString(dbConfig);

        startConfig.startPageParams.host =  pt.get<std::string>("StartPage.host");
        startConfig.startPageParams.port =  pt.get<std::string>("StartPage.port");
        startConfig.startPageParams.target =  pt.get<std::string>("StartPage.target");

        startConfig.recursiveCount =  pt.get<int>("Recursive.recursiveCount");
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

int main(int argc, char **argv) {
    StartConfig startConfig;
    readConfig(startConfig);

    Spider spider;
    try {
        DatabaseManager dbmanager(startConfig.dbConnectionString);

        RequestConfig reqConfig;
        reqConfig.host = startConfig.startPageParams.host;
        reqConfig.port = startConfig.startPageParams.port;
        reqConfig.target = startConfig.startPageParams.target;

        spider.setDbManager(&dbmanager);
        spider.start(reqConfig, startConfig.recursiveCount);
    } catch (const std::exception &e) {
        std::cout << e.what() << std::endl;
    }
}
