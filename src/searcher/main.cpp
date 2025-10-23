#include "http_server.h"
// #include "database.hpp"
#include <iostream>
#include <thread>
#include <vector>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include "../database_manager/database_manager.h"

/**
* @brief Стартовая структура.
*/
struct StartConfig {
    std::string dbConnectionString;
    int port;
};

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

        startConfig.port = pt.get<int>("Server.port");
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

int main() {
    try {
        StartConfig startConfig;
        readConfig(startConfig);

        // TODO
        const int thread_count = 2; // Количество потоков
        DatabaseManager dbmanager(startConfig.dbConnectionString);

        std::cout << "Starting Search Engine Server..." << std::endl;


        net::io_context ioc {thread_count};

        // Создаем endpoint для прослушивания
        auto const address = net::ip::make_address("0.0.0.0");
        tcp::endpoint endpoint {address, static_cast<unsigned short>(startConfig.port)};

        // Создаем и запускаем HTTP сервер
        HTTPServer server(ioc, endpoint, &dbmanager);
        server.start();

        std::cout << "Server running on http://localhost:" << startConfig.port << std::endl;
        std::cout << "Press Ctrl+C to stop" << std::endl;

        // Обработка сигналов для graceful shutdown
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&](boost::system::error_code const &, int) {
            std::cout << "\nShutting down..." << std::endl;
            ioc.stop();
        });

        // Запускаем в нескольких потоках
        std::vector<std::thread> threads;
        threads.reserve(thread_count - 1);

        for (int i = 0; i < thread_count - 1; ++i) {
            threads.emplace_back([&ioc]() { ioc.run(); });
        }

        // Основной поток тоже обрабатывает соединения
        ioc.run();

        // Ждем завершения всех потоков
        for (auto &t : threads) {
            t.join();
        }

        std::cout << "Server stopped" << std::endl;

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
