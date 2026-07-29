#pragma once

#include "server/network/player_id.h"
#include "server/user/user_id.h"
#include "model/piece.h"
#include "server/room/room_id.h"

#include <string>

namespace kfc::matchmaking {

enum class MatchJoinStatus {
    Queued,
    Error,
};

struct MatchResponse {
    MatchJoinStatus status = MatchJoinStatus::Queued;
    std::string message;
};

struct MatchCompletedEvent {
    PlayerId white_player_id = 0;
    UserId white_user_id = 0;
    PlayerId black_player_id = 0;
    UserId black_user_id = 0;
    RoomId room_id = 0;
    std::string server_id;
    std::string endpoint;
};

struct MatchTimeoutEvent {
    PlayerId player_id = 0;
};

}  // namespace kfc::matchmaking
