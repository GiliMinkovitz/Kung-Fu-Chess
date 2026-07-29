#pragma once

#include "matchmaking/protocol/match_notification.h"
#include "matchmaking/protocol/match_request.h"
#include "matchmaking/protocol/match_response.h"

#include <optional>
#include <string>
#include <string_view>

namespace kfc::matchmaking {

[[nodiscard]] std::string encode_match_request(const MatchRequest& request);
[[nodiscard]] std::optional<MatchRequest> decode_match_request(std::string_view body);

[[nodiscard]] std::string encode_match_response(const MatchResponse& response);
[[nodiscard]] std::optional<MatchResponse> decode_match_response(std::string_view body);

[[nodiscard]] std::string encode_match_notification(const MatchNotification& notification);
[[nodiscard]] std::optional<MatchNotification> decode_match_notification(std::string_view body);

[[nodiscard]] std::string encode_timeout_notification(PlayerId player_id);
[[nodiscard]] std::optional<PlayerId> decode_timeout_notification(std::string_view body);

}  // namespace kfc::matchmaking
