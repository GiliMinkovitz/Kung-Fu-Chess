#pragma once

#include "server/game/i_game_host.h"
#include "server/room/room_manager.h"

#include <string>

namespace kfc {

class ClientSessionManager;

class LocalGameHost : public IGameHost {
public:
    LocalGameHost(RoomManager& room_manager, std::string game_server_id);

    void bind_session_manager(ClientSessionManager& session_manager);

    GameCreationResponse create_room(const GameCreationRequest& request) override;

private:
    RoomManager& room_manager_;
    std::string game_server_id_;
    ClientSessionManager* session_manager_ = nullptr;
};

}  // namespace kfc
