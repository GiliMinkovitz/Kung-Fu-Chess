#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace kfc {

enum class LoginResultStatus {
    Ok,
    Failed,
};

struct LoginResult {
    LoginResultStatus status;
    int rating = 0;
    std::string failure_reason;
};

std::optional<LoginResult> read_login_message(std::string_view text);

}  // namespace kfc
