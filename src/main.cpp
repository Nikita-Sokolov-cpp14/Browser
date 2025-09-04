#include <cstdlib>
#include <iostream>
#include <string>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include "spider/spider.h"
#include "spider/indexer/indexer.h"

namespace {

const std::string host = "www.google.com";
const std::string port = "80";
const std::string target = "/";

}

void parse(std::string &page);

// Performs an HTTP GET and prints the response
int main(int argc, char **argv) {
    Spider spider(host, port, target);
    spider.loadPage();
    Indexer indexer;
    indexer.setPage(spider.getResponseStr());

    // std::cout << spider.getResponseStr() << std::endl;
}

void parse(std::string &page) {
    std::cout << page << std::endl;

    std::cout << "-------" << std::endl;

    // Парсим HTML строку
    htmlDocPtr doc = htmlReadMemory(page.c_str(), // исходная строка
            page.length(), // длина строки
            nullptr, // URL (не используется)
            nullptr, // кодировка (автоопределение)
            HTML_PARSE_NOERROR // опции парсинга
    );

    if (!doc) {
        std::cerr << "Ошибка при парсинге HTML" << std::endl;
        return;
    }

    // Удаление тегов
    xmlChar *text = xmlNodeGetContent(xmlDocGetRootElement(doc));
    std::string plainText(reinterpret_cast<char*>(text));
    xmlFree(text);

    std::cout << plainText << std::endl;
}
