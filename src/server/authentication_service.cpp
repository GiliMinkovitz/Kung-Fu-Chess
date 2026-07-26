#include "server/authentication_service.h"

#include "database/player_repository.h"

namespace kfc {

AuthenticationService::AuthenticationService(PlayerRepository& player_repository)
    : player_repository_(player_repository) {}

AuthenticationResult AuthenticationService::authenticate(const std::string& username,
                                                         const std::string& password) {
    (void)password;

    if (username.empty()) {
        return AuthenticationResult{false, std::nullopt, "invalid_username"};
    }

    if (const auto existing = player_repository_.find_by_username(username)) {
        return AuthenticationResult{true, *existing, ""};
    }

    if (const auto created = player_repository_.create_player(username, 1000)) {
        return AuthenticationResult{true, *created, ""};
    }

    return AuthenticationResult{false, std::nullopt, "create_failed"};
}

}  // namespace kfc
