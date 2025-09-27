#include "database_manager.h"

#include "querys.h"

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
                title VARCHAR(255) NOT NULL
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

void DatabaseManager::writeData(const std::string &pageTitle,
        const std::map<std::string, int> &storage) {
    try {
        pqxx::work txn(connection_);

        // Добавляем страницу и получаем её ID
        pqxx::result page_result =
                txn.exec_params("INSERT INTO pages (title) VALUES ($1) RETURNING id", pageTitle);
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

            std::cout << "Добавлено: Page ID: " << page_id << ", Word: " << val.first
                      << ", Count: " << val.second << ", Word ID: " << word_id << std::endl;
        }

        txn.commit();
        std::cout << "Все данные для страницы '" << pageTitle << "' успешно добавлены!"
                  << std::endl;

        txn.commit();

    } catch (const std::exception &e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
    }
}
