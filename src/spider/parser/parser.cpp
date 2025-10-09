#include "parser.h"
#include <iostream>
#include <string>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <boost/locale.hpp>

Parser::Parser() :
sourceStr_(),
text_() {
}

void Parser::parse(const std::string &source) {
    try {
        sourceStr_ = source;
        clearTags();
        clearPunctuation();
        toLowerRegistr();
    } catch (std::exception &err) {
        std::cerr << "Parser::parse: Error: " << err.what() << std::endl;
    }
}

std::string Parser::getText() const {
    return text_;
}

void Parser::clearTags() {
    if (sourceStr_.empty()) {
        text_ = "";
        return;
    }

    // Парсится HTML строка
    htmlDocPtr doc = htmlReadMemory(sourceStr_.c_str(), // исходная строка
            sourceStr_.length(), // длина строки
            nullptr, // URL (не используется)
            nullptr, // кодировка (автоопределение)
            HTML_PARSE_NOERROR // опции парсинга
    );

    if (!doc) {
        std::cerr << "Parser::clearTags: ERROR: Can't parsing HTML" << std::endl;
        return;
    }

    // Удаление тегов
    xmlChar *textWithoutTags = xmlNodeGetContent(xmlDocGetRootElement(doc));
    text_ = reinterpret_cast<char *>(textWithoutTags);
    xmlFree(textWithoutTags);
}

void Parser::clearPunctuation() {
    if (text_.empty()) {
        return;
    }

    for (size_t i = 0; i < text_.size(); ++i) {
        if (ispunct(text_[i])) {
            text_[i] = ' ';
        }
    }
}

void Parser::toLowerRegistr() {
    boost::locale::generator gen;
    std::locale loc = gen("ru_RU.UTF-8");
    std::locale::global(loc);

    text_ = boost::locale::to_lower(text_);
}
