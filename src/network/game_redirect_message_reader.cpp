#include "network/game_redirect_message_reader.h"

#include <sstream>
#include <string>
#include <vector>

namespace kfc {

namespace {

std::string first_line(std::string_view text) {
    std::string line;
    for (const char ch : text) {
        if (ch == '\r') {
            continue;
        }
        if (ch == '\n') {
            break;
        }
        line.push_back(ch);
    }
    return line;
}

std::vector<std::string> split_tokens(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream stream{line};
    std::string token;
    while (stream >> token) {
        tokens.push_back(std::move(token));
    }
    return tokens;
}

std::optional<PieceColor> parse_side_token(const std::string& token) {
    if (token == "white") {
        return PieceColor::White;
    }
    if (token == "black") {
        return PieceColor::Black;
    }
    return std::nullopt;
}

std::optional<RoomId> parse_room_id_token(const std::string& token) {
    if (token.empty()) {
        return std::nullopt;
    }

    RoomId value = 0;
    for (const char ch : token) {
        if (ch < '0' || ch > '9') {
            return std::nullopt;
        }
        value = value * 10 + static_cast<RoomId>(ch - '0');
    }
    return value;
}

}  // namespace

std::optional<GameRedirectInfo> read_game_redirect_message(std::string_view text) {
    const std::vector<std::string> tokens = split_tokens(first_line(text));
    if (tokens.size() != 4 || tokens[0] != "game_redirect") {
        return std::nullopt;
    }

    const std::optional<PieceColor> side = parse_side_token(tokens[3]);
    const std::optional<RoomId> room_id = parse_room_id_token(tokens[2]);
    if (!side.has_value() || !room_id.has_value() || tokens[1].empty()) {
        return std::nullopt;
    }

    GameRedirectInfo redirect_info;
    redirect_info.endpoint = tokens[1];
    redirect_info.room_id = *room_id;
    redirect_info.side = *side;
    return redirect_info;
}

}  // namespace kfc
