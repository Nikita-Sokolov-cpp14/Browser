#pragma once
#include <iostream>
#include <string>
#include "../parser/parser.h"

/**
* @brief Индексатор.
*/
class Indexer {
public:
    Indexer();

    /**
    * @brief Конструктор.
    * @param htmlPage Необработанная HTML строка.
    */
    Indexer(const std::string &htmlPage);

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

private:
    Parser parser; //!< Парсер HTML страницы.

    std::string text; //!< HTML станица в виде строки без тегов и знаков препинания, в нижнем регистре.
};
