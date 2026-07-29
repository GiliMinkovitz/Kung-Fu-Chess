#include "server/game/protocol/game_creation_codec.h"

#include <sstream>

namespace kfc {

namespace {

std::optional<UserId> parse_user_id(std::string_view token) {
    if (token.empty()) {
        return std::nullopt;
    }

    UserId value = 0;
    for (const char ch : token) {
        if (ch < '0' || ch > '9') {
            return std::nullopt;
        }
        value = value * 10 + static_cast<UserId>(ch - '0');
    }
    return value;
}

std::optional<std::string> field_at(std::string_view value, std::size_t index) {
    std::size_t start = 0;
    for (std::size_t current = 0; current <= index; ++current) {
        const std::size_t next = value.find('|', start);
        if (current == index) {
            if (next == std::string_view::npos) {
                return std::string(value.substr(start));
            }
            return std::string(value.substr(start, next - start));
        }
        if (next == std::string_view::npos) {
            return std::nullopt;
        }
        start = next + 1;
    }
    return std::nullopt;
}

}  // namespace

std::string encode_game_creation_request(const GameCreationRequest& request) {
    std::ostringstream out;
    out << request.white_user_id << '|' << request.black_user_id << '|';
    if (request.db_game_id.has_value()) {
        out << *request.db_game_id;
    }
    return out.str();
}

std::optional<GameCreationRequest> decode_game_creation_request(const std::string_view body) {
    const std::optional<std::string> white = field_at(body, 0);
    const std::optional<std::string> black = field_at(body, 1);
    if (!white.has_value() || !black.has_value()) {
        return std::nullopt;
    }

    const std::optional<UserId> white_user_id = parse_user_id(*white);
    const std::optional<UserId> black_user_id = parse_user_id(*black);
    if (!white_user_id.has_value() || !black_user_id.has_value()) {
        return std::nullopt;
    }

    GameCreationRequest request{*white_user_id, *black_user_id, std::nullopt};
    if (const std::optional<std::string> db_game_id = field_at(body, 2)) {
        if (!db_game_id->empty()) {
            try {
                request.db_game_id = std::stoi(*db_game_id);
            } catch (...) {
                return std::nullopt;
            }
        }
    }
    return request;
}

std::string encode_game_creation_response(const GameCreationResponse& response) {
    std::ostringstream out;
    out << response.room_id << '|' << response.game_server_id << '|';
    if (response.endpoint.has_value()) {
        out << *response.endpoint;
    }
    return out.str();
}

std::optional<GameCreationResponse> decode_game_creation_response(const std::string_view body) {
    const std::optional<std::string> room_id_token = field_at(body, 0);
    const std::optional<std::string> server_id = field_at(body, 1);
    if (!room_id_token.has_value() || !server_id.has_value()) {
        return std::nullopt;
    }

    RoomId room_id = 0;
    try {
        room_id = static_cast<RoomId>(std::stoull(*room_id_token));
    } catch (...) {
        return std::nullopt;
    }

    GameCreationResponse response{room_id, *server_id, std::nullopt};
    if (const std::optional<std::string> endpoint = field_at(body, 2)) {
        if (!endpoint->empty()) {
            response.endpoint = *endpoint;
        }
    }
    return response;
}

}  // namespace kfc
