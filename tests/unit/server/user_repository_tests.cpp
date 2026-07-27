#include "server/database/in_memory_user_repository.h"

#include <doctest/doctest.h>

TEST_CASE("InMemoryUserRepositoryTest - CreatesUserWithGeneratedId") {
    kfc::InMemoryUserRepository repository;

    const kfc::UserId first_id = repository.create_user("carol");
    const kfc::UserId second_id = repository.create_user("dave");

    CHECK(first_id != second_id);
    REQUIRE(repository.find_by_id(first_id) != nullptr);
    REQUIRE(repository.find_by_id(second_id) != nullptr);
    CHECK_EQ(repository.find_by_id(first_id)->username(), "carol");
    CHECK_EQ(repository.find_by_id(second_id)->username(), "dave");
}

TEST_CASE("InMemoryUserRepositoryTest - CreatesUserWithExplicitId") {
    kfc::InMemoryUserRepository repository;

    const kfc::UserId id = repository.create_user(7, "bob");
    CHECK_EQ(id, 7u);

    const kfc::User* user = repository.find_by_id(7);
    REQUIRE(user != nullptr);
    CHECK_EQ(user->id(), 7u);
    CHECK_EQ(user->username(), "bob");

    const kfc::User* by_name = repository.find_by_username("bob");
    REQUIRE(by_name != nullptr);
    CHECK_EQ(by_name->id(), 7u);
}

TEST_CASE("InMemoryUserRepositoryTest - ReturnsNullForMissingUser") {
    kfc::InMemoryUserRepository repository;

    CHECK(repository.find_by_id(99) == nullptr);
    CHECK(repository.find_by_username("missing") == nullptr);
}
