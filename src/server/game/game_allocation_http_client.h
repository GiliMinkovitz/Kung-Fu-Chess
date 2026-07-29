#pragma once

#include "server/game/protocol/game_creation_request.h"
#include "server/game/protocol/game_creation_response.h"

#include <chrono>
#include <optional>
#include <string>

namespace kfc {

class GameAllocationHttpClient {
public:
    GameAllocationHttpClient(std::string service_token,
                               std::chrono::milliseconds timeout = std::chrono::milliseconds{1000});

    [[nodiscard]] std::optional<GameCreationResponse> allocate(
        const std::string& allocation_endpoint, const GameCreationRequest& request) const;

private:
    std::string service_token_;
    std::chrono::milliseconds timeout_;
};

}  // namespace kfc
