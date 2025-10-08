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
    std::cout << "sucsess clear tags" << std::endl;
    clearPunctuation();
    std::cout << "sucsess clear clearPunctuation" << std::endl;
    toLowerRegistr();
}

std::string Parser::getText() const {
    return text;
}

std::string Parser::getSourceStr() const {
    return sourceStr;
}

void Parser::clearTags() {
    if (sourceStr.empty()) {
        text = "";
        return;
    }

    // Парсим HTML строку
    htmlDocPtr doc = htmlReadMemory(sourceStr.c_str(), // исходная строка
            sourceStr.length(), // длина строки
            nullptr, // URL (не используется)
            nullptr, // кодировка (автоопределение)
            HTML_PARSE_NOERROR // опции парсинга
    );

    std::cout << "len: " << sourceStr.length() << std::endl;

    if (!doc) {
        std::cerr << "Parser::clearTags: ERROR: Can't parsing HTML" << std::endl;
        return;
    }

    // Удаление тегов
    xmlChar *textWithoutTags = xmlNodeGetContent(xmlDocGetRootElement(doc));
    text = reinterpret_cast<char*>(textWithoutTags);
    xmlFree(textWithoutTags);
}

void Parser::clearPunctuation() {
    // Защита от пустой строки
    if (text.empty()) {
        return;
    }

    // Используем size_t и правильные границы
    for (size_t i = 0; i < text.size(); ++i) {
        for (size_t j = 0; j < punctuationSymbols.size(); ++j) {
            if (text[i] == punctuationSymbols[j]) {
                text[i] = ' ';
                break; // Выходим после нахождения совпадения
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
