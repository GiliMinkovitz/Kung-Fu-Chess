#pragma once

#include "matchmaking/gateway/i_gateway_notifier.h"

#include <chrono>
#include <string>

namespace kfc::matchmaking {

class GatewayNotifierHttpClient final : public IGatewayNotifier {
public:
    GatewayNotifierHttpClient(std::string gateway_notification_endpoint,
                              std::chrono::milliseconds timeout = std::chrono::milliseconds{1000});

    bool notify_match_found(const MatchNotification& notification) override;
    bool notify_search_timeout(PlayerId player_id) override;

private:
    std::string gateway_notification_endpoint_;
    std::chrono::milliseconds timeout_;
};

}  // namespace kfc::matchmaking
