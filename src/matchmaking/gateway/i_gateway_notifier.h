#pragma once

#include "matchmaking/protocol/match_notification.h"

namespace kfc::matchmaking {

class IGatewayNotifier {
public:
    virtual ~IGatewayNotifier() = default;

    virtual bool notify_match_found(const MatchNotification& notification) = 0;
    virtual bool notify_search_timeout(PlayerId player_id) = 0;
};

}  // namespace kfc::matchmaking
