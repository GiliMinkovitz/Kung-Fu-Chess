#pragma once

#include "logic/game_action.h"
#include "server/match.h"
#include "server/player_session.h"

#include "model/piece.h"

#include <optional>
#include <string>
#include <string_view>

namespace kfc {

struct LoginRequest {
    std::string username;
    std::string password;
};

bool parse_play_message(std::string_view message);
bool parse_resign_message(std::string_view message);
std::optional<LoginRequest> parse_login_message(std::string_view message);
std::optional<GameAction> parse_message(std::string_view message);
bool is_action_allowed(PieceColor player_side, const Match& match, const GameAction& action);
bool is_action_allowed(const PlayerSession& session, const Match& match, const GameAction& action);

#ifdef KFC_TEST_BUILD
namespace test {
std::optional<std::size_t> parse_non_negative_int_for_tests(std::string_view token);
}
#endif

}  // namespace kfc
