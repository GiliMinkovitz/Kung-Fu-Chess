#pragma once

namespace kfc {

class ClientSessionManager;
class IUserRepository;
class RoomManager;

class GameJoinHandler {
public:
    GameJoinHandler(RoomManager& room_manager, ClientSessionManager& session_manager,
                    IUserRepository& user_repository);

    void process();

private:
    RoomManager& room_manager_;
    ClientSessionManager& session_manager_;
    IUserRepository& user_repository_;
};

}  // namespace kfc
