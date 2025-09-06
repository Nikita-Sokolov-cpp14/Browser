#include "parser.h"
#include <iostream>
#include <string>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <boost/locale.hpp>

namespace {

//! TODO: Знаки (){}[]_+=|'&/<> Тоже надо удалять?
//! Строка со знаками препинания.
const std::string punctuationSymbols = ".,;:!?{([})]=_|'&/<>\n\t\b";

}

Parser::Parser() :
sourceStr(),
text() {
}

void Parser::parse(const std::string &source) {
    sourceStr = source;
    clearTags();
    clearPunctuation();
    toLowerRegistr();
}

std::string Parser::getText() const {
    return text;
}

std::string Parser::getSourceStr() const {
    return sourceStr;
}

void Parser::clearTags() {
    // Парсим HTML строку
    htmlDocPtr doc = htmlReadMemory(sourceStr.c_str(), // исходная строка
            sourceStr.length(), // длина строки
            nullptr, // URL (не используется)
            nullptr, // кодировка (автоопределение)
            HTML_PARSE_NOERROR // опции парсинга
    );

    if (!doc) {
        std::cerr << "Ошибка при парсинге HTML" << std::endl;
        return;
    }

    // Удаление тегов
    xmlChar *textWithoutTags = xmlNodeGetContent(xmlDocGetRootElement(doc));
    text = reinterpret_cast<char*>(textWithoutTags);
    xmlFree(textWithoutTags);
}

void Parser::clearPunctuation() {
    for (int i = 0; i < text.size() - 1; ++i) {
        for (int j = 0; j < punctuationSymbols.size() - 1; ++j) {
            if (text[i] == punctuationSymbols[j]) {
                text[i] = ' ';
            }
        }
    }
}

void Parser::toLowerRegistr() {
    // Инициализация локали
    boost::locale::generator gen;
    std::locale loc = gen("ru_RU.UTF-8");
    std::locale::global(loc);

    text = boost::locale::to_lower(text);
}
