#pragma once

#include "matchmaking/protocol/match_request.h"
#include "matchmaking/protocol/match_response.h"
#include "server/network/player_id.h"

#include <optional>
#include <string_view>

namespace kfc::matchmaking {

class IMatchmakingJoinClient {
public:
    virtual ~IMatchmakingJoinClient() = default;

    [[nodiscard]] virtual std::optional<MatchResponse> join(const MatchRequest& request) = 0;
    virtual void leave(PlayerId player_id, std::string_view region) = 0;
};

}  // namespace kfc::matchmaking
