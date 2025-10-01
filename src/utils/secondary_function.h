#pragma once

#include <iostream>
#include <vector>

#include "../common_data.h"

RequestConfig parseUrl(const std::string &url);

void extractAllLinks(const std::string &htmlContent, std::vector<RequestConfig> &links);
