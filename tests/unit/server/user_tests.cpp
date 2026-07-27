#include "server/user/user.h"
#include "server/user/user_registry.h"

#include <doctest/doctest.h>

TEST_CASE("UserTest - StoresIdentity") {
    kfc::User user{42, "alice"};

    CHECK_EQ(user.id(), 42u);
    CHECK_EQ(user.username(), "alice");
}

TEST_CASE("UserRegistryTest - RegistersUserWithExplicitId") {
    kfc::UserRegistry registry;

    const kfc::UserId id = registry.register_user(7, "bob");
    CHECK_EQ(id, 7u);

    const kfc::User* user = registry.find(7);
    REQUIRE(user != nullptr);
    CHECK_EQ(user->id(), 7u);
    CHECK_EQ(user->username(), "bob");

    const kfc::User* by_name = registry.find_by_username("bob");
    REQUIRE(by_name != nullptr);
    CHECK_EQ(by_name->id(), 7u);
}

TEST_CASE("UserRegistryTest - CreatesUserWithGeneratedId") {
    kfc::UserRegistry registry;

    const kfc::UserId first_id = registry.create_user("carol");
    const kfc::UserId second_id = registry.create_user("dave");

    CHECK(first_id != second_id);
    REQUIRE(registry.find(first_id) != nullptr);
    REQUIRE(registry.find(second_id) != nullptr);
    CHECK_EQ(registry.find(first_id)->username(), "carol");
    CHECK_EQ(registry.find(second_id)->username(), "dave");
}

TEST_CASE("UserRegistryTest - ReturnsNullForMissingUser") {
    kfc::UserRegistry registry;

    CHECK(registry.find(99) == nullptr);
    CHECK(registry.find_by_username("missing") == nullptr);
}
