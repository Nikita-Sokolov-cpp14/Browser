#pragma once

#include <iostream>
#include <string>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>

class Parser {
public:
    Parser();

    void parse(const std::string &sourceStr);

    std::string getText() const;

    std::string getSourceStr() const;

private:
    std::string sourceStr;
    std::string text;

};
