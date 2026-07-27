#include "server/database/postgres_user_repository.h"

#include <cstdlib>
#include <string>

#if KFC_HAS_LIBPQ
#include <libpq-fe.h>
#endif

namespace kfc {

namespace {

#if KFC_HAS_LIBPQ

struct PgResultGuard {
    PGresult* result = nullptr;

    ~PgResultGuard() {
        if (result != nullptr) {
            PQclear(result);
        }
    }
};

bool connection_ok(PGconn* connection) {
    return connection != nullptr && PQstatus(connection) == CONNECTION_OK;
}

bool result_ok(PGresult* result) {
    if (result == nullptr) {
        return false;
    }

    const ExecStatusType status = PQresultStatus(result);
    return status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK;
}

std::optional<Player> row_to_player(PGresult* result, int row) {
    if (PQgetisnull(result, row, 0) || PQgetisnull(result, row, 1) ||
        PQgetisnull(result, row, 2)) {
        return std::nullopt;
    }

    const char* id_text = PQgetvalue(result, row, 0);
    const char* username = PQgetvalue(result, row, 1);
    const char* rating_text = PQgetvalue(result, row, 2);
    if (id_text == nullptr || username == nullptr || rating_text == nullptr) {
        return std::nullopt;
    }

    return Player(std::atoi(id_text), username, std::atoi(rating_text));
}

struct StoredCredentials {
    int id;
    std::string username;
    int rating;
    std::string password_hash;
};

std::optional<StoredCredentials> row_to_credentials(PGresult* result, int row) {
    if (PQgetisnull(result, row, 0) || PQgetisnull(result, row, 1) ||
        PQgetisnull(result, row, 2)) {
        return std::nullopt;
    }

    const char* id_text = PQgetvalue(result, row, 0);
    const char* username = PQgetvalue(result, row, 1);
    const char* rating_text = PQgetvalue(result, row, 2);
    const char* password_hash = PQgetisnull(result, row, 3) ? "" : PQgetvalue(result, row, 3);
    if (id_text == nullptr || username == nullptr || rating_text == nullptr) {
        return std::nullopt;
    }

    return StoredCredentials{
        static_cast<int>(std::strtol(id_text, nullptr, 10)),
        username,
        static_cast<int>(std::strtol(rating_text, nullptr, 10)),
        password_hash != nullptr ? password_hash : "",
    };
}

PGresult* exec_params(PGconn* connection, const char* sql, int param_count,
                      const char* const* param_values) {
    return PQexecParams(connection, sql, param_count, nullptr, param_values, nullptr, nullptr, 0);
}

std::optional<Player> find_player_by_id(PGconn* connection, int player_id) {
    const std::string id = std::to_string(player_id);
    const char* params[] = {id.c_str()};

    PgResultGuard guard;
    guard.result = exec_params(
        connection,
        "SELECT id, username, rating FROM players WHERE id = $1 LIMIT 1;",
        1,
        params);
    if (!result_ok(guard.result) || PQntuples(guard.result) == 0) {
        return std::nullopt;
    }

    return row_to_player(guard.result, 0);
}

std::optional<StoredCredentials> find_credentials(PGconn* connection,
                                                  const std::string& username) {
    const char* params[] = {username.c_str()};

    PgResultGuard guard;
    guard.result = exec_params(
        connection,
        "SELECT id, username, rating, password_hash FROM players WHERE username = $1 LIMIT 1;",
        1,
        params);
    if (!result_ok(guard.result) || PQntuples(guard.result) == 0) {
        return std::nullopt;
    }

    return row_to_credentials(guard.result, 0);
}

std::optional<Player> insert_player(PGconn* connection, const std::string& username, int rating,
                                    const std::string& password_hash) {
    const std::string rating_text = std::to_string(rating);
    const char* hash_param = password_hash.empty() ? nullptr : password_hash.c_str();
    const char* params[] = {username.c_str(), rating_text.c_str(), hash_param};

    PgResultGuard guard;
    guard.result = exec_params(
        connection,
        "INSERT INTO players (username, rating, password_hash) VALUES ($1, $2, $3) "
        "RETURNING id;",
        3,
        params);
    if (!result_ok(guard.result) || PQntuples(guard.result) == 0 ||
        PQgetisnull(guard.result, 0, 0)) {
        return std::nullopt;
    }

    const char* id_text = PQgetvalue(guard.result, 0, 0);
    if (id_text == nullptr) {
        return std::nullopt;
    }

    return Player(static_cast<int>(std::strtol(id_text, nullptr, 10)), username, rating);
}

bool update_player_rating(PGconn* connection, int player_id, int rating) {
    const std::string rating_text = std::to_string(rating);
    const std::string id_text = std::to_string(player_id);
    const char* params[] = {rating_text.c_str(), id_text.c_str()};

    PgResultGuard guard;
    guard.result = exec_params(connection, "UPDATE players SET rating = $1 WHERE id = $2;", 2,
                               params);
    if (!result_ok(guard.result)) {
        return false;
    }

    const char* affected = PQcmdTuples(guard.result);
    return affected != nullptr && std::strtol(affected, nullptr, 10) > 0;
}

#endif

}  // namespace

PostgresUserRepository::PostgresUserRepository(PostgresConnection& database)
    : database_(database) {}

UserId PostgresUserRepository::create_user(std::string username) {
    if (const auto created = create_user_with_password(std::move(username), 1000, "")) {
        return static_cast<UserId>(created->id());
    }
    return 0;
}

UserId PostgresUserRepository::create_user(const UserId id, std::string username) {
    if (find_by_id(id) != nullptr) {
        return id;
    }
    if (const auto created = create_user_with_password(std::move(username), 1000, "")) {
        return static_cast<UserId>(created->id());
    }
    return id;
}

const User* PostgresUserRepository::cache_user(const UserId id,
                                               const std::string& username) const noexcept {
    cached_user_ = User(id, username);
    return &*cached_user_;
}

const User* PostgresUserRepository::find_by_id(const UserId id) const noexcept {
    if (const auto profile = find_profile_by_id(id)) {
        return cache_user(id, profile->username());
    }
    return nullptr;
}

const User* PostgresUserRepository::find_by_username(const std::string& username) const noexcept {
    if (const auto profile = find_credentials_by_username(username)) {
        return cache_user(profile->id, profile->username);
    }
    return nullptr;
}

std::optional<UserCredentials> PostgresUserRepository::find_credentials_by_username(
    const std::string& username) const {
#if KFC_HAS_LIBPQ
    PGconn* connection = database_.native_connection();
    if (!connection_ok(connection)) {
        return std::nullopt;
    }

    if (const auto credentials = find_credentials(connection, username)) {
        return UserCredentials{
            static_cast<UserId>(credentials->id),
            credentials->username,
            credentials->rating,
            credentials->password_hash,
        };
    }
#else
    (void)username;
#endif
    return std::nullopt;
}

std::optional<Player> PostgresUserRepository::find_profile_by_id(const UserId id) const {
#if KFC_HAS_LIBPQ
    PGconn* connection = database_.native_connection();
    if (!connection_ok(connection)) {
        return std::nullopt;
    }

    return find_player_by_id(connection, static_cast<int>(id));
#else
    (void)id;
#endif
    return std::nullopt;
}

std::optional<Player> PostgresUserRepository::create_user_with_password(
    std::string username, int rating, std::string password_hash) {
#if KFC_HAS_LIBPQ
    PGconn* connection = database_.native_connection();
    if (!connection_ok(connection)) {
        return std::nullopt;
    }

    return insert_player(connection, username, rating, password_hash);
#else
    (void)username;
    (void)rating;
    (void)password_hash;
#endif
    return std::nullopt;
}

bool PostgresUserRepository::update_rating(const UserId id, int rating) {
#if KFC_HAS_LIBPQ
    PGconn* connection = database_.native_connection();
    if (!connection_ok(connection)) {
        return false;
    }

    return update_player_rating(connection, static_cast<int>(id), rating);
#else
    (void)id;
    (void)rating;
#endif
    return false;
}

}  // namespace kfc
