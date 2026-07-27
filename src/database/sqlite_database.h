#pragma once

#include "database/i_database_connection.h"

#include <sqlite3.h>

#include <string>

namespace kfc {

class SqliteDatabase final : public IDatabaseConnection {
public:
    explicit SqliteDatabase(const std::string& path);
    ~SqliteDatabase() override;

    bool open();
    bool initialize_schema();

    sqlite3* connection() override;

private:
    std::string path_;
    sqlite3* db_{nullptr};
};

}  // namespace kfc
