#include "secondary_function.h"

#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>

RequestConfig parseUrl(const std::string &url) {
    RequestConfig config;

    // Если ссылка пустая, возвращаем пустую структуру
    if (url.empty()) {
        std::cerr << "parseUrl: url is empty" << std::endl;
        return config;
    }

    // Находим позицию начала хоста (после ://)
    size_t scheme_end = url.find("://");
    if (scheme_end == std::string::npos) {
        std::cerr << "parseUrl: no scheme found in URL: " << url << std::endl;
        return config;
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
        return RequestConfig{}; // возвращаем пустой объект
    }

    return config;
}

void extractAllLinks(const std::string &htmlContent, std::vector<RequestConfig> &links) {
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
                RequestConfig config = parseUrl(url);

                // std::cout << url << std::endl;

                if (!config.host.empty()) {
                    links.push_back(config);
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
