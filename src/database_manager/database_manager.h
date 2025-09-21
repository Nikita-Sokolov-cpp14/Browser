#pragma once

#include <pqxx/pqxx>
#include <string>
#include <iostream>
#include <map>

class DatabaseManager {
public:
    DatabaseManager(const std::string &connectionString);
    ~DatabaseManager();

    void clearDatabase();

    void writeData(const std::string &pageTitle, const std::map<std::string, int> &storage);

private:
    pqxx::connection connection_;
};
