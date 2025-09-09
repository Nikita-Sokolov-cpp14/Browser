#include "indexer.h"

Indexer::Indexer() :
parser(),
text() {
}

Indexer::Indexer(const std::string &htmlPage) :
parser(),
text() {
    setPage(htmlPage);
}

void Indexer::setPage(const std::string &htmlPage) {
    parser.parse(htmlPage);
    text = parser.getText();
    calcCountWords();
}

std::string Indexer::getText() {
    return text;
}

Indexer::Storage Indexer::getStorage() const {
    return storage;
}

void Indexer::calcCountWords() {
    std::string word;

    for (int i = 0; i < text.length(); ++i) {
        if (text[i] == ' ' && i != text.length() - 1) {
            ++i;
            word = "";
            while (text[i] != ' ' && i < text.length()) {
                word += text[i];
                ++i;
            }

            if (word == "\"" || word == "") {
                continue;
            }

            storage[word]++;
        }
    }
}
