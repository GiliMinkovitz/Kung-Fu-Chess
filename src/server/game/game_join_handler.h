#pragma once

#include "server/room/room_manager.h"
#include "server/session/client_session_manager.h"

namespace kfc {

class GameJoinHandler {
public:
    GameJoinHandler(RoomManager& room_manager, ClientSessionManager& session_manager);

    void process();

private:
    RoomManager& room_manager_;
    ClientSessionManager& session_manager_;
};

}  // namespace kfc
