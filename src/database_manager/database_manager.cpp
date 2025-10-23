#include "database_manager.h"

DatabaseManager::DatabaseManager(const std::string &connectionString) :
connection_(connectionString) {
    if (!connection_.is_open()) {
        throw std::runtime_error("DatabaseManager::DatabaseManager: Error connect");
    } else {
        std::cout << "DatabaseManager::DatabaseManager: sucsessful connection" << std::endl;
    }
}

DatabaseManager::~DatabaseManager() {
    if (connection_.is_open()) {
        connection_.disconnect();
    }
}

void DatabaseManager::createTables() {
    try {
        pqxx::work txn(connection_);

        // Создание таблицы pages
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS pages (
                id SERIAL PRIMARY KEY,
                host VARCHAR(127) NOT NULL,
                port VARCHAR(31) NOT NULL,
                target VARCHAR(255) NOT NULL
            )
        )");
        std::cout << "DatabaseManager::createTables: Table 'pages' created" << std::endl;

        // Создание таблицы words
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS words (
                id_word SERIAL PRIMARY KEY,
                word VARCHAR(50) NOT NULL,
                word_count INT NOT NULL
            )
        )");
        std::cout << "DatabaseManager::createTables: Table 'words' created" << std::endl;

        // Создание связующей таблицы page_words
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS page_words (
                id SERIAL PRIMARY KEY,
                page_id INT NOT NULL,
                word_id INT NOT NULL,

                FOREIGN KEY (page_id) REFERENCES pages(id),
                FOREIGN KEY (word_id) REFERENCES words(id_word),
                UNIQUE (page_id, word_id)
            )
        )");
        std::cout << "DatabaseManager::createTables: Table 'page_words' created" << std::endl;

        // Коммитим транзакцию
        txn.commit();
        std::cout << "DatabaseManager::createTables: All tables created" << std::endl;

    } catch (const std::exception &e) {
        std::cerr << "DatabaseManager::createTables: Error create tables " << e.what() << std::endl;
    }
}

void DatabaseManager::clearDatabase() {
    try {
        pqxx::work tx(connection_);

        tx.exec("TRUNCATE TABLE page_words, pages, words RESTART IDENTITY;");

        tx.commit();
        std::cout << "DatabaseManager::clearDatabase: sucsessful clear db" << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "DatabaseManager::clearDatabase: Error clear db: " << e.what() << std::endl;
    }
}

void DatabaseManager::writeData(const RequestConfig &requestConfig,
        const std::map<std::string, int> &storage) {
    try {
        pqxx::work txn(connection_);

        // Добавляем страницу и получаем её ID
        pqxx::result page_result =
            txn.exec_params(
                "INSERT INTO pages (host, port, target) VALUES ($1, $2, $3) RETURNING id",
                requestConfig.host,
                requestConfig.port,
                requestConfig.target
        );
        int page_id = page_result[0][0].as<int>();

        for (auto &val : storage) {
            if (val.first.length() > 45) {
                continue;
            }

            // Добавляем слово и получаем его ID
            pqxx::result new_word_result = txn.exec_params(
                    "INSERT INTO words (word, word_count) VALUES ($1, $2) RETURNING id_word",
                    val.first, val.second);
            int word_id = new_word_result[0][0].as<int>();

            // Добавляем связь в связующую таблицу
            txn.exec_params("INSERT INTO page_words (page_id, word_id) VALUES ($1, $2) "
                            "ON CONFLICT (page_id, word_id) DO NOTHING",
                    page_id, word_id);

            // std::cout << "Добавлено: Page ID: " << page_id << ", Word: " << val.first
            //           << ", Count: " << val.second << ", Word ID: " << word_id << std::endl;
        }

        txn.commit();
        std::cout << "DatabaseManager::writeData: Все данные для страницы '" <<
            requestConfig.port << requestConfig.target << "' успешно добавлены!"
                  << std::endl;

        txn.commit();

    } catch (const std::exception &e) {
        std::cerr << "DatabaseManager::writeData: Ошибка: " << e.what() << std::endl;
    }
}

void DatabaseManager::searchWords(std::map<int, std::string, std::greater<int>> &results,
        const std::vector<std::string> &words) {

    std::map<std::string, int> titleRelevant;

    for (int i = 0; i < words.size();++ i) {
        std::vector<std::pair<std::string, int> > wordResults;
        getPagesByWord(words[i], wordResults);
        for (auto &val : wordResults) {
            // std::cout << val.first << " " << val.second << std::endl;
            titleRelevant[val.first] += val.second;
        }
    }

    for (auto &val : titleRelevant) {
        results[val.second] = val.first;
    }

    std::cout << "DatabaseManager::searchWords: sucsess" << std::endl;
}

void DatabaseManager::getPagesByWord(const std::string &targetWord,
        std::vector<std::pair<std::string, int> > &results) {
    try {
        pqxx::work txn(connection_);

        std::string query = R"(
            SELECT DISTINCT p.title, w.word_count
            FROM pages p
            JOIN page_words pw ON p.id = pw.page_id
            JOIN words w ON pw.word_id = w.id_word
            WHERE w.word = $1
        )";

        pqxx::result result = txn.exec_params(query, targetWord);

        for (const auto &row : result) {
            std::string word = row[0].as<std::string>();
            int count = row[1].as<int>();
            results.push_back(std::make_pair(word, count));
        }

        txn.commit();

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
