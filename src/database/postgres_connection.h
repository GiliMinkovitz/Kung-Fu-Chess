#pragma once

#include "database/i_database_connection.h"

#if KFC_HAS_LIBPQ
#include <libpq-fe.h>
#else
struct pg_conn;
using PGconn = pg_conn;
#endif

#include <string>

namespace kfc {

class PostgresConnection final : public IDatabaseConnection {
public:
    struct Settings {
        std::string host;
        int port = 5432;
        std::string database;
        std::string username;
        std::string password;
    };

    explicit PostgresConnection(Settings settings);
    ~PostgresConnection() override;

    PostgresConnection(const PostgresConnection&) = delete;
    PostgresConnection& operator=(const PostgresConnection&) = delete;

    bool open();
    bool initialize_schema();

    sqlite3* connection() override;
    [[nodiscard]] PGconn* native_connection() const noexcept;

private:
    Settings settings_;
#if KFC_HAS_LIBPQ
    PGconn* connection_{nullptr};
#endif
};

}  // namespace kfc
