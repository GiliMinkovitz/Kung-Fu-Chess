#include "server/database/postgres_user_repository.h"

#include "database/postgres_connection.h"

#include <doctest/doctest.h>

TEST_CASE("PostgresUserRepositoryTest - ReturnsNulloptWithoutDatabaseConnection") {
    kfc::PostgresConnection connection{kfc::PostgresConnection::Settings{
        "127.0.0.1",
        1,
        "missing_database",
        "missing_user",
        "missing_password",
    }};
    kfc::PostgresUserRepository repository{connection};

    CHECK(repository.find_by_id(1) == nullptr);
    CHECK(repository.find_by_username("missing") == nullptr);
    CHECK_FALSE(repository.find_profile_by_id(1).has_value());
    CHECK_FALSE(repository.find_credentials_by_username("missing").has_value());
    CHECK_EQ(repository.create_user("new_user"), 0u);
    CHECK_EQ(repository.create_user(7, "explicit"), 7u);
    CHECK_FALSE(repository.create_user_with_password("secure", 1000, "hash").has_value());
    CHECK_FALSE(repository.update_rating(1, 1200));
}

TEST_CASE("PostgresUserRepositoryTest - ReturnsNulloptWhenConnectionOpenFailed") {
    kfc::PostgresConnection connection{kfc::PostgresConnection::Settings{
        "127.0.0.1",
        1,
        "missing_database",
    }};
    CHECK_FALSE(connection.open());

    kfc::PostgresUserRepository repository{connection};

    CHECK(repository.find_by_id(1) == nullptr);
    CHECK_FALSE(repository.create_user_with_password("alice", 1000, "hash").has_value());
    CHECK_FALSE(repository.update_rating(1, 1100));
}
