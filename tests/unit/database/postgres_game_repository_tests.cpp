#include "database/postgres_game_repository.h"

#include "database/postgres_connection.h"

#include <doctest/doctest.h>

TEST_CASE("PostgresGameRepositoryTest - ReturnsNulloptWithoutDatabaseConnection") {
    kfc::PostgresConnection connection{kfc::PostgresConnection::Settings{
        "127.0.0.1",
        1,
        "missing_database",
        "missing_user",
        "missing_password",
    }};
    kfc::PostgresGameRepository repository{connection};

    CHECK_FALSE(repository.create_game(1, 2).has_value());
    CHECK_FALSE(repository.finish_game(1, 1));
    CHECK_FALSE(repository.finish_game_without_winner(1));
}

TEST_CASE("PostgresGameRepositoryTest - ReturnsNulloptWhenConnectionOpenFailed") {
    kfc::PostgresConnection connection{kfc::PostgresConnection::Settings{
        "127.0.0.1",
        1,
        "missing_database",
    }};
    CHECK_FALSE(connection.open());

    kfc::PostgresGameRepository repository{connection};

    CHECK_FALSE(repository.create_game(3, 4).has_value());
    CHECK_FALSE(repository.finish_game(2, 3));
    CHECK_FALSE(repository.finish_game_without_winner(2));
}
