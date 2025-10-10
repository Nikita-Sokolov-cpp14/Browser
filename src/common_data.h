#pragma once

#include <iostream>

/**
* @brief Параметры запроса HTML страницы.
*/
struct RequestConfig {
    std::string host; //!< Хост.
    std::string port; //!< Порт.
    std::string target; //!< Таргет.
};
