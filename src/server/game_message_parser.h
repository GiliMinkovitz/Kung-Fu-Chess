#pragma once

#include "logic/game_action.h"
#include "server/match.h"
#include "server/player_session.h"

#include <optional>
#include <string>
#include <string_view>

namespace kfc {

bool parse_play_message(std::string_view message);
std::optional<std::string> parse_login_message(std::string_view message);
std::optional<GameAction> parse_message(std::string_view message);
bool is_action_allowed(const PlayerSession& session, const Match& match, const GameAction& action);

#ifdef KFC_TEST_BUILD
namespace test {
std::optional<std::size_t> parse_non_negative_int_for_tests(std::string_view token);
}
#endif

}  // namespace kfc
