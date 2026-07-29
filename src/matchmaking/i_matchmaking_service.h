#pragma once

#include "matchmaking/protocol/match_request.h"
#include "matchmaking/protocol/match_response.h"
#include "matchmaking/queued_player.h"

#include <string_view>
#include <vector>

namespace kfc::matchmaking {

class IMatchmakingService {
public:
    virtual ~IMatchmakingService() = default;

    virtual MatchResponse enter_queue(const MatchRequest& request) = 0;
    virtual void leave_queue(PlayerId player_id, std::string_view region) = 0;
    virtual void process_queue(std::string_view region) = 0;
    [[nodiscard]] virtual std::vector<PlayerId> drain_timeouts(std::string_view region) = 0;
    [[nodiscard]] virtual std::size_t waiting_count(std::string_view region) const = 0;
};

}  // namespace kfc::matchmaking
