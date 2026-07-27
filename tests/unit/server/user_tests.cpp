#include "server/user/user.h"

#include <doctest/doctest.h>

TEST_CASE("UserTest - StoresIdentity") {
    kfc::User user{42, "alice"};

    CHECK_EQ(user.id(), 42u);
    CHECK_EQ(user.username(), "alice");
}
