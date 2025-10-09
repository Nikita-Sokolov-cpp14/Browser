#pragma once

#include <iostream>
#include <vector>

#include "../common_data.h"

/**
* @brief Преобразовать строку с URL в структуру.
* @param url Строка с исходным URL.
* @return Структура подключения.
*/
RequestConfig parseUrl(const std::string &url);

/**
* @brief Извлеч все ссылки с HTML страницы.
* @param htmlContent Строка с исходным URL.
* @param links Контейнер для записи ссылок.
*/
void extractAllLinks(const std::string &htmlContent, std::vector<RequestConfig> &links);

/**
* @brief Преобразовать структуру с параметрами подключения в строку.
* @param requestConfig Структура с параметрами подключения.
* @return Строка с параметрами подключения.
*/
std::string convertRequestConfigToTitleString(const RequestConfig &requestConfig);
