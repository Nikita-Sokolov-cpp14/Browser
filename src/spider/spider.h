#pragma once

#include <cstdlib>
#include <iostream>
#include <string>
#include <queue>

std::string getPage(const std::string &host, const std::string &port, const std::string &target);

/**
* @brief Класс программы «Паук».
* @details HTTP-клиент, задача которого — парсить сайты и строить индексы,
* исходя из частоты слов в документах.
*/
class Spider {
public:
    Spider();

    void process(const std::string &startHost, const std::string &startPort,
            const std::string &startTarget);

    /**
    * @brief Конструктор.
    * @param host Хост.
    * @param port Порт.
    * @param target Таргет.
    */
    // Spider(const std::string &host, const std::string &port, const std::string &target);

    /**
    * @brief Скачать HMTL страницу.
    */
    void loadPage(const std::string &host, const std::string &port, const std::string &target);

    /**
    * @brief Получить HTML страницу в виде строки.
    * @return Получить HTML страницу - ответ на запрос.
    */
    std::string &getResponseStr();

private:
    std::string responseStr_; //!< Строка с ответом на запрос.
    std::queue<std::string> queueReferences_;
};
