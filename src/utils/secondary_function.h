#pragma once

#include <iostream>
#include <vector>

#include "../common_data.h"

/**
* @brief Преобразовать строку с URL в структуру.
* @param url Строка с исходным URL.
* @return Параметры запроса HTML страницы.
*/
RequestConfig parseUrl(const std::string &url, const RequestConfig &sourceConfig);

/**
* @brief Извлеч все ссылки с HTML страницы.
* @param htmlContent Строка с исходным URL.
* @param links Контейнер для записи ссылок.
*/
void extractAllLinks(const std::string &htmlContent, std::vector<RequestConfig> &targetLinks,
        const RequestConfig &sourceConfig);

/**
* @brief Преобразовать структуру с параметрами подключения в строку.
* @param requestConfig Структура с параметрами подключения.
* @return Строка с параметрами подключения.
*/
// std::string convertRequestConfigToTitleString(const RequestConfig &requestConfig);
