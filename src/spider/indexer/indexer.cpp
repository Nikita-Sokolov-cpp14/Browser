#include "indexer.h"

#include <pqxx/pqxx>

Indexer::Indexer() :
parser_(),
text_() {
}

// Indexer::Indexer(const std::string &htmlPage, const std::string &host) :
// parser_(),
// storage_(),
// host_(host),
// text_() {
//     setPage(htmlPage, host_);
// }

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

Indexer::Storage Indexer::getStorage() const {
    return storage_;
}

void Indexer::calcCountWords() {
    std::string word;

    for (int i = 0; i < text_.length(); ++i) {
        if (text_[i] == ' ' && i != text_.length() - 1) {
            ++i;
            word = "";
            while (text_[i] != ' ' && i < text_.length()) {
                word += text_[i];
                ++i;
            }

            if (word == "\"" || word == "") {
                continue;
            }

            storage_[word]++;
        }
    }
}
