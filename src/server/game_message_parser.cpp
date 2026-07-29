#include "server/game_message_parser.h"

#include "model/piece_token.h"

#include <cctype>
#include <iostream>
#include <optional>
#include <string_view>
#include <type_traits>
#include <variant>
#ifdef KFC_TEST_BUILD
#include "test/game_message_parser_test_hooks.h"
#endif

namespace {

std::optional<std::string_view> next_token(std::string_view& message) {
    while (!message.empty() && std::isspace(static_cast<unsigned char>(message.front()))) {
        message.remove_prefix(1);
    }
    if (message.empty()) {
        return std::nullopt;
    }

    const std::size_t end = message.find_first_of(" \t\r\n", 0);
    const std::string_view token =
        end == std::string_view::npos ? message : message.substr(0, end);
    message.remove_prefix(token.size());
    return token;
}

std::optional<std::size_t> parse_non_negative_int(std::string_view token) {
    if (token.empty()) {
        return std::nullopt;
    }

    std::size_t value = 0;
    for (const char ch : token) {
        if (!std::isdigit(static_cast<unsigned char>(ch))) {
            return std::nullopt;
        }
        value = value * 10 + static_cast<std::size_t>(ch - '0');
    }
    return value;
}

std::optional<kfc::PieceColor> piece_color_at(const kfc::GameState& state, std::size_t row,
                                             std::size_t col) {
    if (!state.is_piece(row, col)) {
        return std::nullopt;
    }

    std::optional<kfc::PieceDescriptor> descriptor =
        kfc::descriptor_from_token(state.token_at(row, col));
#ifdef KFC_TEST_BUILD
    if (kfc::test::GameMessageParserTestHooks::force_invalid_piece_descriptor) {
        descriptor = std::nullopt;
    }
#endif
    if (!descriptor) {
        return std::nullopt;
    }
    return descriptor->color;
}

}  // namespace

namespace kfc {

bool parse_play_message(std::string_view message) {
    while (!message.empty() && std::isspace(static_cast<unsigned char>(message.front()))) {
        message.remove_prefix(1);
    }
    while (!message.empty() && std::isspace(static_cast<unsigned char>(message.back()))) {
        message.remove_suffix(1);
    }

    if (message.empty()) {
        return false;
    }

    const auto command = next_token(message);
    if (!command || *command != "play") {
        return false;
    }
    if (next_token(message)) {
        return false;
    }

    return true;
}

std::optional<RoomId> parse_join_game_message(std::string_view message) {
    while (!message.empty() && std::isspace(static_cast<unsigned char>(message.front()))) {
        message.remove_prefix(1);
    }
    while (!message.empty() && std::isspace(static_cast<unsigned char>(message.back()))) {
        message.remove_suffix(1);
    }

    if (message.empty()) {
        return std::nullopt;
    }

    const auto command = next_token(message);
    if (!command || *command != "join_game") {
        return std::nullopt;
    }

    const auto room_id_token = next_token(message);
    if (!room_id_token) {
        return std::nullopt;
    }

    const std::optional<std::size_t> parsed_room_id = parse_non_negative_int(*room_id_token);
    if (!parsed_room_id.has_value()) {
        return std::nullopt;
    }
    if (next_token(message)) {
        return std::nullopt;
    }

    return static_cast<RoomId>(*parsed_room_id);
}

bool parse_resign_message(std::string_view message) {
    while (!message.empty() && std::isspace(static_cast<unsigned char>(message.front()))) {
        message.remove_prefix(1);
    }
    while (!message.empty() && std::isspace(static_cast<unsigned char>(message.back()))) {
        message.remove_suffix(1);
    }

    if (message.empty()) {
        return false;
    }

    const auto command = next_token(message);
    if (!command || *command != "resign") {
        return false;
    }
    if (next_token(message)) {
        return false;
    }

    return true;
}

std::optional<LoginRequest> parse_login_message(std::string_view message) {
    while (!message.empty() && std::isspace(static_cast<unsigned char>(message.front()))) {
        message.remove_prefix(1);
    }
    while (!message.empty() && std::isspace(static_cast<unsigned char>(message.back()))) {
        message.remove_suffix(1);
    }

    if (message.empty()) {
        return std::nullopt;
    }

    const auto command = next_token(message);
    if (!command || *command != "login") {
        return std::nullopt;
    }

    const auto username = next_token(message);
    if (!username || username->empty()) {
        return std::nullopt;
    }

    LoginRequest request;
    request.username = std::string(*username);

    if (const auto password = next_token(message)) {
        request.password = std::string(*password);
        if (next_token(message)) {
            return std::nullopt;
        }
    }

    return request;
}

std::optional<GameAction> parse_message(std::string_view message) {
    while (!message.empty() && std::isspace(static_cast<unsigned char>(message.front()))) {
        message.remove_prefix(1);
    }
    while (!message.empty() && std::isspace(static_cast<unsigned char>(message.back()))) {
        message.remove_suffix(1);
    }

    if (message.empty()) {
#ifndef KFC_TEST_BUILD
        std::cerr << "Error: empty message\n";
#endif
        return std::nullopt;
    }

    const auto command = next_token(message);

    if (*command == "clear") {
        if (next_token(message)) {
#ifndef KFC_TEST_BUILD
            std::cerr << "Error: 'clear' takes no arguments\n";
#endif
            return std::nullopt;
        }
        return ClearSelection{};
    }

    if (*command == "select" || *command == "move" || *command == "jump") {
        const auto row = next_token(message);
        const auto col = next_token(message);
        if (!row || !col) {
#ifndef KFC_TEST_BUILD
            std::cerr << "Error: '" << *command << "' requires row and col\n";
#endif
            return std::nullopt;
        }
        if (next_token(message)) {
#ifndef KFC_TEST_BUILD
            std::cerr << "Error: too many arguments for '" << *command << "'\n";
#endif
            return std::nullopt;
        }

        const auto parsed_row = parse_non_negative_int(*row);
        const auto parsed_col = parse_non_negative_int(*col);
        if (!parsed_row || !parsed_col) {
#ifndef KFC_TEST_BUILD
            std::cerr << "Error: row and col must be non-negative integers\n";
#endif
            return std::nullopt;
        }

        if (*command == "select") {
            return Select{*parsed_row, *parsed_col};
        }
        if (*command == "move") {
            return MoveSelected{*parsed_row, *parsed_col};
        }
        return JumpAt{*parsed_row, *parsed_col};
    }

#ifndef KFC_TEST_BUILD
    std::cerr << "Error: unknown command '" << *command << "'\n";
#endif
    return std::nullopt;
}

bool is_action_allowed(const PieceColor player_side, const Match& match, const GameAction& action) {
    const GameState& state = match.state();

    return std::visit(
        [&](const auto& a) -> bool {
            using T = std::decay_t<decltype(a)>;
            if constexpr (std::is_same_v<T, ClearSelection>) {
                return true;
            }
            if constexpr (std::is_same_v<T, AdvanceClock>) {
                return false;
            }
            if constexpr (std::is_same_v<T, Select> || std::is_same_v<T, JumpAt>) {
                const std::optional<PieceColor> piece_color = piece_color_at(state, a.row, a.col);
                return piece_color.has_value() && *piece_color == player_side;
            }
            if constexpr (std::is_same_v<T, MoveSelected> || std::is_same_v<T, JumpSelected>) {
                std::size_t row = 0;
                std::size_t col = 0;
                if (!state.selection(row, col)) {
                    return false;
                }
                const std::optional<PieceColor> piece_color = piece_color_at(state, row, col);
                return piece_color.has_value() && *piece_color == player_side;
            }
            return false;
        },
        action);
}

bool is_action_allowed(const PlayerSession& session, const Match& match, const GameAction& action) {
    if (!session.has_side()) {
        return false;
    }

    return is_action_allowed(session.side(), match, action);
}

}  // namespace kfc

#ifdef KFC_TEST_BUILD
namespace kfc::test {

std::optional<std::size_t> parse_non_negative_int_for_tests(std::string_view token) {
    return parse_non_negative_int(token);
}

}  // namespace kfc::test
#endif
