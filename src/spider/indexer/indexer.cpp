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

}
