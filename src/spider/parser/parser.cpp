#include "parser.h"

Parser::Parser() :
sourceStr(),
text() {
}

void Parser::parse(const std::string &sourceStr) {
    this->sourceStr = sourceStr;
}

std::string Parser::getText() const {
    return text;
}

std::string Parser::getSourceStr() const {
    return sourceStr;
}
