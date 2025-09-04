#pragma once
#include <iostream>
#include <string>
#include "../parser/parser.h"

class Indexer {
public:
    Indexer();

    Indexer(const std::string &htmlPage);

    void setPage(const std::string &htmlPage);

private:
    Parser parser;

    std::string text;
};
