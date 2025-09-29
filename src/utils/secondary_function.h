#pragma once

#include <iostream>

#include "../spider/spider.h"

RequestConfig parseUrl(const std::string &url);

void extractAllLinks(const std::string &htmlContent, std::vector<RequestConfig> &links);
