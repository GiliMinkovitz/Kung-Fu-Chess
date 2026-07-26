#include "server/password_hasher.h"

#include <doctest/doctest.h>

TEST_CASE("PasswordHasherTest - HashAndVerifyRoundTrip") {
    const std::string stored = kfc::PasswordHasher::hash_password("secret");

    CHECK(kfc::PasswordHasher::verify_password("secret", stored));
    CHECK_FALSE(kfc::PasswordHasher::verify_password("wrong", stored));
}

TEST_CASE("PasswordHasherTest - ProducesDistinctHashesForSamePassword") {
    const std::string first = kfc::PasswordHasher::hash_password("secret");
    const std::string second = kfc::PasswordHasher::hash_password("secret");

    CHECK_NE(first, second);
    CHECK(kfc::PasswordHasher::verify_password("secret", first));
    CHECK(kfc::PasswordHasher::verify_password("secret", second));
}

TEST_CASE("PasswordHasherTest - RejectsMalformedStoredHash") {
    CHECK_FALSE(kfc::PasswordHasher::verify_password("secret", "not-a-valid-hash"));
    CHECK_FALSE(kfc::PasswordHasher::verify_password("secret", "pbkdf2_sha256$abc$salt$hash"));
}
