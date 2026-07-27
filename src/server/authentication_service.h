#pragma once

#include "server/database/i_user_repository.h"
#include "server/player.h"

#include <optional>
#include <string>

namespace kfc {

struct AuthenticationResult {
    bool success = false;
    std::optional<Player> player;
    std::string failure_reason;
};

class AuthenticationService {
public:
    explicit AuthenticationService(IUserRepository& user_repository);

    [[nodiscard]] AuthenticationResult authenticate(const std::string& username,
                                                    const std::string& password);

private:
    IUserRepository& user_repository_;
};

}  // namespace kfc
