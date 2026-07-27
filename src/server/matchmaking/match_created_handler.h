#pragma once

#include "server/room/room.h"

namespace kfc {

class PlayerSession;

struct MatchCreated {
    PlayerSession* white = nullptr;
    PlayerSession* black = nullptr;
    RoomId room_id = 0;
};

class IMatchCreatedHandler {
public:
    virtual ~IMatchCreatedHandler() = default;
    virtual RoomId create_match(PlayerSession* white, PlayerSession* black) = 0;
};

}  // namespace kfc
