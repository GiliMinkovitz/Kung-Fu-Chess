#include "server/game/game_allocation_handler.h"

#include "app/i_runtime_store.h"

namespace kfc {

GameAllocationHandler::GameAllocationHandler(IGameHost& game_host, IRuntimeStore& runtime_store,
                                             std::string server_id, std::string endpoint)
    : game_host_(game_host),
      runtime_store_(runtime_store),
      server_id_(std::move(server_id)),
      endpoint_(std::move(endpoint)) {}

GameCreationResponse GameAllocationHandler::allocate(const GameCreationRequest& request) {
    GameCreationResponse response = game_host_.create_room(request);
    if (!response.endpoint.has_value() || response.endpoint->empty()) {
        response.endpoint = endpoint_;
    }
    if (response.game_server_id.empty()) {
        response.game_server_id = server_id_;
    }

    runtime_store_.register_room(response.room_id, request.white_user_id, request.black_user_id,
                               response.game_server_id, *response.endpoint);
    return response;
}

}  // namespace kfc
