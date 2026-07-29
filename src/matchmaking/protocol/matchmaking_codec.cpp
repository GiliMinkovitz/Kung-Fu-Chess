#include "matchmaking/protocol/matchmaking_codec.h"

#include <sstream>

namespace kfc::matchmaking {

namespace {

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

template <typename T>
std::optional<T> parse_integral(std::string_view token) {
    if (token.empty()) {
        return std::nullopt;
    }
    T value = 0;
    for (const char ch : token) {
        if (ch < '0' || ch > '9') {
            return std::nullopt;
        }
        value = static_cast<T>(value * 10 + static_cast<T>(ch - '0'));
    }
    return value;
}

}  // namespace

std::string encode_match_request(const MatchRequest& request) {
    std::ostringstream out;
    out << request.player_id << '|' << request.user_id << '|' << request.elo << '|'
        << request.region;
    return out.str();
}

std::optional<MatchRequest> decode_match_request(const std::string_view body) {
    const std::optional<std::string> player_id = field_at(body, 0);
    const std::optional<std::string> user_id = field_at(body, 1);
    const std::optional<std::string> elo = field_at(body, 2);
    const std::optional<std::string> region = field_at(body, 3);
    if (!player_id.has_value() || !user_id.has_value() || !elo.has_value() ||
        !region.has_value()) {
        return std::nullopt;
    }

    const std::optional<PlayerId> parsed_player_id = parse_integral<PlayerId>(*player_id);
    const std::optional<UserId> parsed_user_id = parse_integral<UserId>(*user_id);
    if (!parsed_player_id.has_value() || !parsed_user_id.has_value()) {
        return std::nullopt;
    }

    try {
        return MatchRequest{*parsed_player_id, *parsed_user_id, std::stoi(*elo), *region};
    } catch (...) {
        return std::nullopt;
    }
}

std::string encode_match_response(const MatchResponse& response) {
    std::ostringstream out;
    out << (response.status == MatchJoinStatus::Queued ? "queued" : "error") << '|'
        << response.message;
    return out.str();
}

std::optional<MatchResponse> decode_match_response(const std::string_view body) {
    const std::optional<std::string> status = field_at(body, 0);
    if (!status.has_value()) {
        return std::nullopt;
    }

    MatchResponse response;
    if (*status == "queued") {
        response.status = MatchJoinStatus::Queued;
    } else if (*status == "error") {
        response.status = MatchJoinStatus::Error;
    } else {
        return std::nullopt;
    }

    if (const std::optional<std::string> message = field_at(body, 1)) {
        response.message = *message;
    }
    return response;
}

std::string encode_match_notification(const MatchNotification& notification) {
    std::ostringstream out;
    out << notification.white_player_id << '|' << notification.white_user_id << '|'
        << notification.black_player_id << '|' << notification.black_user_id << '|'
        << notification.room_id << '|' << notification.server_id << '|'
        << notification.endpoint;
    return out.str();
}

std::optional<MatchNotification> decode_match_notification(const std::string_view body) {
    const std::optional<std::string> white_player_id = field_at(body, 0);
    const std::optional<std::string> white_user_id = field_at(body, 1);
    const std::optional<std::string> black_player_id = field_at(body, 2);
    const std::optional<std::string> black_user_id = field_at(body, 3);
    const std::optional<std::string> room_id = field_at(body, 4);
    const std::optional<std::string> server_id = field_at(body, 5);
    const std::optional<std::string> endpoint = field_at(body, 6);
    if (!white_player_id.has_value() || !white_user_id.has_value() ||
        !black_player_id.has_value() || !black_user_id.has_value() || !room_id.has_value() ||
        !server_id.has_value() || !endpoint.has_value()) {
        return std::nullopt;
    }

    MatchNotification notification;
    try {
        notification.white_player_id = static_cast<PlayerId>(std::stoull(*white_player_id));
        notification.white_user_id = static_cast<UserId>(std::stoull(*white_user_id));
        notification.black_player_id = static_cast<PlayerId>(std::stoull(*black_player_id));
        notification.black_user_id = static_cast<UserId>(std::stoull(*black_user_id));
        notification.room_id = static_cast<RoomId>(std::stoull(*room_id));
        notification.server_id = *server_id;
        notification.endpoint = *endpoint;
    } catch (...) {
        return std::nullopt;
    }
    return notification;
}

std::string encode_timeout_notification(const PlayerId player_id) {
    return "timeout|" + std::to_string(player_id);
}

std::optional<PlayerId> decode_timeout_notification(const std::string_view body) {
    const std::optional<std::string> kind = field_at(body, 0);
    const std::optional<std::string> player_id = field_at(body, 1);
    if (!kind.has_value() || *kind != "timeout" || !player_id.has_value()) {
        return std::nullopt;
    }
    return parse_integral<PlayerId>(*player_id);
}

}  // namespace kfc::matchmaking
