#pragma once

#include <iostream>
#include <string>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>

/**
* @brief Парсер.
* @details Очищает строку от HTML тегов и знаков препинания, переводит все слова в
* нижний регистр.
*/
class Parser {
public:
    /**
    * @brief Конструктор.
    */
    Parser();

    /**
    * @brief Выполнить парсинг исходной необработанной HTML страницы в виде строки.
    * @param source Необработанная HTML страница.
    */
    void parse(const std::string &source);

    /**
    * @brief Получить обработанную HTML страницу в виде строки.
    * @return Обработанная HTML страница в виде строки.
    */
    std::string getText() const;

private:
    std::string sourceStr_; //!< Исходная HTML страница в виде строки.
    std::string text_; //!< Преобразованная HTML страница.

    /**
    * @brief Очистить HTML страницу от тегов.
    */
    void clearTags();

    /**
    * @brief Очистить HTML страницу от знаков препинания.
    */
    void clearPunctuation();

    /**
    * @brief Перевести слова HTML страницы в нижний регистр.
    */
    void toLowerRegistr();
};
