#pragma once

#include <sqlite3.h>

#include <string>

namespace kfc {

class SqliteDatabase {
public:
    explicit SqliteDatabase(const std::string& path);
    ~SqliteDatabase();

    bool open();
    bool initialize_schema();

    sqlite3* connection();

private:
    std::string path_;
    sqlite3* db_{nullptr};
};

}  // namespace kfc
