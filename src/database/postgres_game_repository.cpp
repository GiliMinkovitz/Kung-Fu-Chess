#include "database/postgres_game_repository.h"

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

PGresult* exec_params(PGconn* connection, const char* sql, int param_count,
                      const char* const* param_values) {
    return PQexecParams(connection, sql, param_count, nullptr, param_values, nullptr, nullptr, 0);
}

#endif

}  // namespace

PostgresGameRepository::PostgresGameRepository(PostgresConnection& database)
    : database_(database) {}

std::optional<int> PostgresGameRepository::create_game(int white_player_id, int black_player_id) {
#if KFC_HAS_LIBPQ
    PGconn* connection = database_.native_connection();
    if (!connection_ok(connection)) {
        return std::nullopt;
    }

    const std::string white = std::to_string(white_player_id);
    const std::string black = std::to_string(black_player_id);
    const char* params[] = {white.c_str(), black.c_str()};

    PgResultGuard guard;
    guard.result = exec_params(
        connection,
        "INSERT INTO games (white_player_id, black_player_id, status, created_at) "
        "VALUES ($1, $2, 'active', NOW()) RETURNING id;",
        2,
        params);
    if (!result_ok(guard.result) || PQntuples(guard.result) == 0 ||
        PQgetisnull(guard.result, 0, 0)) {
        return std::nullopt;
    }

    const char* id_text = PQgetvalue(guard.result, 0, 0);
    if (id_text == nullptr) {
        return std::nullopt;
    }

    return static_cast<int>(std::strtol(id_text, nullptr, 10));
#else
    (void)white_player_id;
    (void)black_player_id;
#endif
    return std::nullopt;
}

bool PostgresGameRepository::finish_game(int game_id, int winner_player_id) {
#if KFC_HAS_LIBPQ
    PGconn* connection = database_.native_connection();
    if (!connection_ok(connection)) {
        return false;
    }

    const std::string winner = std::to_string(winner_player_id);
    const std::string game = std::to_string(game_id);
    const char* params[] = {winner.c_str(), game.c_str()};

    PgResultGuard guard;
    guard.result = exec_params(
        connection,
        "UPDATE games SET status = 'finished', winner_id = $1 WHERE id = $2;",
        2,
        params);
    return result_ok(guard.result);
#else
    (void)game_id;
    (void)winner_player_id;
#endif
    return false;
}

bool PostgresGameRepository::finish_game_without_winner(int game_id) {
#if KFC_HAS_LIBPQ
    PGconn* connection = database_.native_connection();
    if (!connection_ok(connection)) {
        return false;
    }

    const std::string game = std::to_string(game_id);
    const char* params[] = {game.c_str()};

    PgResultGuard guard;
    guard.result = exec_params(
        connection,
        "UPDATE games SET status = 'finished', winner_id = NULL WHERE id = $1;",
        1,
        params);
    return result_ok(guard.result);
#else
    (void)game_id;
#endif
    return false;
}

}  // namespace kfc
