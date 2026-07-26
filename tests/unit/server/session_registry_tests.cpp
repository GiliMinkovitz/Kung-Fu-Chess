#include "server/session_registry.h"

#include <doctest/doctest.h>

TEST_CASE("SessionRegistryTest - RegisterAndUnregister") {
    kfc::SessionRegistry registry;

    CHECK(registry.register_session("alice"));
    CHECK(registry.is_online("alice"));
    CHECK_FALSE(registry.is_online("bob"));

    registry.unregister_session("alice");
    CHECK_FALSE(registry.is_online("alice"));
}

TEST_CASE("SessionRegistryTest - DetectsDuplicateRegistration") {
    kfc::SessionRegistry registry;

    CHECK(registry.register_session("alice"));
    CHECK_FALSE(registry.register_session("alice"));
    CHECK(registry.is_online("alice"));
}

TEST_CASE("SessionRegistryTest - RejectsEmptyUsername") {
    kfc::SessionRegistry registry;

    CHECK_FALSE(registry.register_session(""));
    CHECK_FALSE(registry.is_online(""));
}

TEST_CASE("SessionRegistryTest - UnregisterIsIdempotent") {
    kfc::SessionRegistry registry;

    registry.unregister_session("missing");
    CHECK(registry.register_session("alice"));
    registry.unregister_session("alice");
    registry.unregister_session("alice");
    CHECK_FALSE(registry.is_online("alice"));
}
