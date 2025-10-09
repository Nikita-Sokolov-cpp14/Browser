#pragma once
#include <iostream>
#include <string>
#include <map>
#include <pqxx/pqxx>

#include "../parser/parser.h"
#include "../database_manager/database_manager.h"
#include "../common_data.h"

/**
* @brief Индексатор.
*/
class Indexer {
public:
    //TODO Вынести отдельно. Используется и в классе БД.
    //! Тип хранилища счетчика слов.
    typedef std::map<std::string, int> Storage;

    /**
    * @brief Конструктор.
    */
    Indexer();

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

    /**
    * @brief Сохранить запись в БД.
    * @param dbManager Ссылка на объект БД.
    * @param requestConfig Параметры подключения к странице.
    */
    void saveDataToDb(DatabaseManager &dbManager, const RequestConfig &requestConfig);

private:
    Parser parser_; //!< Парсер HTML страницы.
    Storage storage_; //!< Хранилище счетчика слов.
    //! HTML станица в виде строки без тегов и знаков препинания, в нижнем регистре.
    std::string text_;

    /**
    * @brief Посчитать количество каждого слова в строке.
    */
    void calcCountWords();
};
