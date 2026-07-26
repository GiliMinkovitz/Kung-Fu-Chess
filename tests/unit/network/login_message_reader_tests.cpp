#include "network/login_message_reader.h"

#include <doctest/doctest.h>

TEST_CASE("LoginMessageReaderTest - ParsesLoginOk") {
    const auto result = kfc::read_login_message("login_ok 1100");
    REQUIRE(result.has_value());
    CHECK(result->status == kfc::LoginResultStatus::Ok);
    CHECK_EQ(result->rating, 1100);
}

TEST_CASE("LoginMessageReaderTest - ParsesLoginFailed") {
    const auto result = kfc::read_login_message("login_failed already_connected");
    REQUIRE(result.has_value());
    CHECK(result->status == kfc::LoginResultStatus::Failed);
    CHECK_EQ(result->failure_reason, "already_connected");
}

TEST_CASE("LoginMessageReaderTest - ParsesMultiWordFailureReason") {
    const auto result = kfc::read_login_message("login_failed invalid credentials");
    REQUIRE(result.has_value());
    CHECK(result->status == kfc::LoginResultStatus::Failed);
    CHECK_EQ(result->failure_reason, "invalid credentials");
}

TEST_CASE("LoginMessageReaderTest - IgnoresUnrelatedMessages") {
    CHECK_FALSE(kfc::read_login_message("searching").has_value());
    CHECK_FALSE(kfc::read_login_message("login_ok").has_value());
    CHECK_FALSE(kfc::read_login_message("login_ok not_a_number").has_value());
}
