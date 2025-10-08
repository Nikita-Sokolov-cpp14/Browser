#pragma once
#include <iostream>
#include <string>
#include <map>
#include <pqxx/pqxx>

#include "../parser/parser.h"
#include "../database_manager/database_manager.h"

/**
* @brief Индексатор.
*/
class Indexer {
public:
    //TODO Вынести отдельно. Используется и в классе БД.
    typedef std::map<std::string, int> Storage;

    Indexer();

    // /**
    // * @brief Конструктор.
    // * @param htmlPage Необработанная HTML строка.
    // */
    // Indexer(const std::string &htmlPage, const std::string &host);

    /**
    * @brief Установить HTML страницу.
    * @details Очищает HTML страницу от знаков тегов и знаков препинания.
    * @param htmlPage Необработанная HTML строка.
    */
    void setPage(const std::string &htmlPage);

    /**
    * @brief Получить обработанную HTML страницу.
    * @return обработанная HTML страница.
    */
    std::string getText();

    Storage getStorage() const;

    //TODO
    void setConnection(const pqxx::connection &con);

    void saveDataToDb(DatabaseManager &dbManager, const std::string &host);

private:
    Parser parser_; //!< Парсер HTML страницы.

    Storage storage_;
    std::string host_;

    //! HTML станица в виде строки без тегов и знаков препинания, в нижнем регистре.
    std::string text_;

    void calcCountWords();
};
