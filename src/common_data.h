#pragma once

#include <iostream>

/**
* @brief Структура подключения  к HTML странице.
*/
struct RequestConfig {
    std::string host; //!< Хост.
    std::string port; //!< Порт.
    std::string target; //!< Таргет.
};
