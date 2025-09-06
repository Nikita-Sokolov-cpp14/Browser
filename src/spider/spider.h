#pragma once

#include <cstdlib>
#include <iostream>
#include <string>

/**
* @brief Класс программы «Паук».
* @details HTTP-клиент, задача которого — парсить сайты и строить индексы,
* исходя из частоты слов в документах.
*/
class Spider {
public:
    Spider();

    /**
    * @brief Конструктор.
    * @param host Хост.
    * @param port Порт.
    * @param target Таргет.
    */
    Spider(const std::string &host, const std::string &port, const std::string &target);

    /**
    * @brief Скачать HMTL страницу.
    */
    void loadPage();

    /**
    * @brief Получить HTML страницу в виде строки.
    * @return Получить HTML страницу - ответ на запрос.
    */
    std::string &getResponseStr();

private:
    std::string host;
    std::string port;
    std::string target;
    std::string responseStr; //!< Строка с ответом на запрос.
};
