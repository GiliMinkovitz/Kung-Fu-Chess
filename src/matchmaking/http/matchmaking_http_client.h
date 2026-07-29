#pragma once

#include "matchmaking/i_matchmaking_join_client.h"
#include "matchmaking/protocol/match_request.h"
#include "matchmaking/protocol/match_response.h"

#include <chrono>
#include <optional>
#include <string>

namespace kfc::matchmaking {

class MatchmakingHttpClient final : public IMatchmakingJoinClient {
public:
    MatchmakingHttpClient(std::string matchmaker_endpoint,
                            std::chrono::milliseconds timeout = std::chrono::milliseconds{1000});

    [[nodiscard]] std::optional<MatchResponse> join(const MatchRequest& request) override;
    void leave(PlayerId player_id, std::string_view region) override;

private:
    std::string matchmaker_endpoint_;
    std::chrono::milliseconds timeout_;
};

}  // namespace kfc::matchmaking
