#include "indexer.h"

#include <pqxx/pqxx>

Indexer::Indexer() :
parser_(),
storage_(),
text_() {
}

void Indexer::setPage(const std::string &htmlPage) {
    parser_.parse(htmlPage);
    text_ = parser_.getText();
    calcCountWords();
}

void Indexer::saveDataToDb(DatabaseManager &dbManager, const std::string &host) {
    dbManager.writeData(host, storage_);
}

std::string Indexer::getText() {
    return text_;
}

void Indexer::calcCountWords() {
    std::string word;

    for (int i = 0; i < text_.length(); ++i) {
        if (text_[i] == ' ') {
            continue;
        }

        word = "";
        while (i < text_.length() && text_[i] != ' ') {
            word += text_[i];
            ++i;
        }
        storage_[word]++;
    }
}
