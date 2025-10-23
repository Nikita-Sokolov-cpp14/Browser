#include "secondary_function.h"

#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>

namespace {

// RequestConfig parseRelativeLinks(const std::string &url, const RequestConfig &sourceConfig) {
//     if (url.substr(0, 2) == "./") {
//         RequestConfig config = sourceConfig;
//         int startPathPos = 0;
//         for (int i = 0; i < url.size() - 1; ++i) {
//             if (url[i] == '/' && url[i + 1] != '.') {
//                 startPathPos = i;
//             }
//         }
//         config.target = url.substr(startPathPos);
//         return config;
//     } else {
//         RequestConfig config = sourceConfig;
//         const std::string path = sourceConfig.target;
//         const int endPath = path.find('?');
//         std::string sourcePath;
//         sourcePath = endPath != path.npos ? path.substr(0, endPath) : path;

//         int pos = url.find_last_of("../");
//         if (pos == url.npos) {
//             config.target = sourcePath + url;
//             return config;
//         }

//         int count = (pos + 3) / 3;
//         while (count > 0) {
//             int size = sourcePath.find_first_of('/');
//             if (size == sourcePath.npos) {
//                 break;
//             }
//             sourcePath = sourcePath.substr(0, size + 1);
//         }
//         config.target = sourcePath + url.substr(pos + 3);
//     }

//     return RequestConfig();
// }

RequestConfig parseRelativeLinks(const std::string &url, const RequestConfig &sourceConfig) {
    RequestConfig config = sourceConfig;

    // Если URL начинается с ./, просто убираем этот префикс
    if (url.substr(0, 2) == "./") {
        config.target = url.substr(2);
        return config;
    }

    const std::string path = sourceConfig.target;
    const size_t endPath = path.find('?');
    std::string sourcePath = endPath != std::string::npos ? path.substr(0, endPath) : path;

    // Обработка случаев с ../
    if (url.find("..") != std::string::npos) {
        // Подсчитываем количество ../ в начале URL
        size_t pos = 0;
        int parentCount = 0;
        while (pos + 2 < url.size() && url.substr(pos, 3) == "../") {
            parentCount++;
            pos += 3;
        }

        // Удаляем соответствующее количество сегментов из sourcePath
        std::string currentPath = sourcePath;
        for (int i = 0; i < parentCount; i++) {
            size_t lastSlash = currentPath.find_last_of('/');
            if (lastSlash == std::string::npos || lastSlash == 0) {
                // Достигли корня
                currentPath = "/";
                break;
            }
            currentPath = currentPath.substr(0, lastSlash);
        }

        // Добавляем оставшуюся часть URL
        std::string remainingUrl = url.substr(pos);
        if (!remainingUrl.empty() && remainingUrl[0] != '/') {
            config.target = currentPath + "/" + remainingUrl;
        } else {
            config.target = currentPath + remainingUrl;
        }

        return config;
    }

    // Простой случай - относительный путь без ../
    // Берем исходный путь как есть (без обрезки последнего сегмента)
    if (sourcePath == "/") {
        sourcePath = "";
    }
    config.target = sourcePath + (url.empty() || url[0] == '/' ? "" : "/") + url;
    return config;
}

} // namespace

RequestConfig parseUrl(const std::string &url, const RequestConfig &sourceConfig) {
    RequestConfig config;

    // Если ссылка пустая, возвращаем пустую структуру
    if (url.empty()) {
        std::cerr << "parseUrl: url is empty" << std::endl;
        return config;
    }

    // Находим позицию начала хоста (после ://)
    size_t scheme_end = url.find("://");
    if (scheme_end == std::string::npos) {
        std::cerr << "parseUrl: parse relative links: " << url << std::endl;
        if (url[0] == '#') {
            std::cerr << "parseUrl: url is fragment: " << std::endl;
            return RequestConfig();
        }

        return parseRelativeLinks(url, sourceConfig);
    }

    std::string scheme = url.substr(0, scheme_end);
    size_t host_start = scheme_end + 3; // пропускаем "://"

    // Находим конец хоста (до порта или пути)
    size_t host_end = url.find('/', host_start);

    std::string hostPort;
    if (host_end == std::string::npos) {
        // Нет пути - весь остаток URL это host:port
        hostPort = url.substr(host_start);
    } else {
        hostPort = url.substr(host_start, host_end - host_start);
    }

    // Обрабатываем host:port
    size_t portStart = hostPort.find(':');
    bool portIsExist = (portStart != std::string::npos);

    if (!portIsExist) {
        config.host = hostPort;
    } else {
        config.host = hostPort.substr(0, portStart);
        config.port = hostPort.substr(portStart + 1);
    }

    // Устанавливаем путь
    if (host_end != std::string::npos) {
        config.target = url.substr(host_end);
    } else {
        config.target = "/"; // путь по умолчанию
    }

    // Устанавливаем порт по умолчанию если не найден
    if (!portIsExist) {
        if (scheme == "https") {
            config.port = "443";
        } else {
            config.port = "80";
        }
    }

    // Валидация результата
    if (config.host.empty()) {
        std::cerr << "parseUrl: empty host in URL: " << url << std::endl;
        return RequestConfig {}; // возвращаем пустой объект
    }

    return config;
}

void extractAllLinks(const std::string &htmlContent, std::vector<RequestConfig> &targetLinks,
        const RequestConfig &sourceConfig) {
    htmlDocPtr doc = htmlReadDoc(reinterpret_cast<const xmlChar *>(htmlContent.c_str()), nullptr,
            nullptr, HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);

    if (doc == nullptr) {
        std::cerr << "extractAllLinks: Can't parsing HTML" << std::endl;
        return;
    }

    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
    if (xpathCtx == nullptr) {
        std::cerr << "extractAllLinks: Error creating XPath context" << std::endl;
        xmlFreeDoc(doc);
        return;
    }

    // Более специфичный XPath для тегов <a>
    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression(BAD_CAST "//a/@href", xpathCtx);

    if (xpathObj == nullptr || xpathObj->nodesetval == nullptr) {
        std::cerr << "extractAllLinks: Error XPath request or ref not found" << std::endl;
        xmlXPathFreeContext(xpathCtx);
        xmlFreeDoc(doc);
        return;
    }

    int size = xpathObj->nodesetval->nodeNr;
    for (int i = 0; i < size; ++i) {
        xmlNodePtr attrNode = xpathObj->nodesetval->nodeTab[i];

        if (attrNode->type == XML_ATTRIBUTE_NODE) {
            xmlChar *hrefValue = xmlNodeGetContent(attrNode);
            if (hrefValue != nullptr) {
                std::string url = reinterpret_cast<const char *>(hrefValue);
                RequestConfig config = parseUrl(url, sourceConfig);
                // std::cout << sourceConfig.host << " " << sourceConfig.target << std::endl;
                // std::cout << config.host << " " << config.target << std::endl;

                if (!config.host.empty()) {
                    targetLinks.push_back(config);
                }
                xmlFree(hrefValue);
            }
        }
    }

    xmlXPathFreeObject(xpathObj);
    xmlXPathFreeContext(xpathCtx);
    xmlFreeDoc(doc);
}

std::string convertRequestConfigToTitleString(const RequestConfig &requestConfig) {
    std::string titleString = "host:" + requestConfig.host + " ";
    titleString += "port:" + requestConfig.port + " ";
    titleString += "target:" + requestConfig.target;

    return titleString;
}
