#pragma once

#include "server/player.h"

#include <optional>
#include <string>

namespace kfc {

class PlayerRepository;

struct AuthenticationResult {
    bool success = false;
    std::optional<Player> player;
    std::string failure_reason;
};

class AuthenticationService {
public:
    explicit AuthenticationService(PlayerRepository& player_repository);

    [[nodiscard]] AuthenticationResult authenticate(const std::string& username,
                                                    const std::string& password);

private:
    PlayerRepository& player_repository_;
};

}  // namespace kfc
