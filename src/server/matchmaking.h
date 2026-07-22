#pragma once

#include "server/player_session.h"

#include <optional>
#include <vector>

namespace kfc {

class Matchmaking {
public:
    void enqueue(PlayerSession& session);
    [[nodiscard]] std::optional<std::vector<PlayerSession*>> try_create_match();

private:
    std::vector<PlayerSession*> waiting_;
};

}  // namespace kfc
