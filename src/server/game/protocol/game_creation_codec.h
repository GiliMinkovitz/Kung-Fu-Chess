#pragma once

#include "server/game/protocol/game_creation_request.h"
#include "server/game/protocol/game_creation_response.h"

#include <optional>
#include <string>
#include <string_view>

namespace kfc {

[[nodiscard]] std::string encode_game_creation_request(const GameCreationRequest& request);
[[nodiscard]] std::optional<GameCreationRequest> decode_game_creation_request(std::string_view body);

[[nodiscard]] std::string encode_game_creation_response(const GameCreationResponse& response);
[[nodiscard]] std::optional<GameCreationResponse> decode_game_creation_response(std::string_view body);

}  // namespace kfc
