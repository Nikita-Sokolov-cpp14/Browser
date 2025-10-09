#pragma once

#include <pqxx/pqxx>
#include <string>
#include <iostream>
#include <map>

/**
* @brief Класс взаимодействия с БД PostgeSql.
*/
class DatabaseManager {
public:
    /**
    * @brief Конструктор.
    * @param connectionString Строка с параметрами подключения к БД,
    */
    DatabaseManager(const std::string &connectionString);

    /**
    * @brief Деструктор.
    */
    ~DatabaseManager();

    /**
    * @brief ОСоздать все необходимые таблицы.
    */
    void createTables();

    /**
    * @brief Очистить БД от данных.
    */
    void clearDatabase();

    /**
    * @brief Деструктор.
    */
    void writeData(const std::string &pageTitle, const std::map<std::string, int> &storage);

private:
    //! Объект подключения к БД PostgreSql
    pqxx::connection connection_;
};
