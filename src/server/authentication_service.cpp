#include "server/authentication_service.h"

#include "server/password_hasher.h"

namespace kfc {

AuthenticationService::AuthenticationService(IUserRepository& user_repository)
    : user_repository_(user_repository) {}

AuthenticationResult AuthenticationService::authenticate(const std::string& username,
                                                         const std::string& password) {
    if (username.empty()) {
        return AuthenticationResult{false, std::nullopt, "invalid_username"};
    }

    if (password.empty()) {
        return AuthenticationResult{false, std::nullopt, "missing_password"};
    }

    if (const auto existing = user_repository_.find_credentials_by_username(username)) {
        if (existing->password_hash.empty()) {
            return AuthenticationResult{false, std::nullopt, "invalid_password"};
        }

        if (!PasswordHasher::verify_password(password, existing->password_hash)) {
            return AuthenticationResult{false, std::nullopt, "invalid_password"};
        }

        return AuthenticationResult{
            true, Player(static_cast<int>(existing->id), existing->username, existing->rating), ""};
    }

    const std::string password_hash = PasswordHasher::hash_password(password);
    if (const auto created =
            user_repository_.create_user_with_password(username, 1000, password_hash)) {
        return AuthenticationResult{true, *created, ""};
    }

    if (user_repository_.find_credentials_by_username(username)) {
        return AuthenticationResult{false, std::nullopt, "username_taken"};
    }

    return AuthenticationResult{false, std::nullopt, "create_failed"};
}

}  // namespace kfc
