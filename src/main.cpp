#include <cstdlib>
#include <iostream>
#include <string>
#include "spider/spider.h"
#include "spider/indexer/indexer.h"

//! Тестовые данные для скачивания страницы. Это загружаемый аналог стартовой страницы.
namespace {

const std::string host = "www.google.com";
const std::string port = "80";
const std::string target = "/";

}

int main(int argc, char **argv) {
    Spider spider(host, port, target);
    spider.loadPage();

    //! TODO: Индексатор пока отдельно, но в будущем будет внутри Spider.
    Indexer indexer;
    indexer.setPage(spider.getResponseStr());

    // Проверка резултатов. Должа вывестись строка без HTML тегов, без знаков препинания и в нижнем регистре
    std::cout << indexer.getText() << std::endl;
}
