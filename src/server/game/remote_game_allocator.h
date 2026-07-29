#pragma once

#include "app/game_server_record.h"
#include "server/game/i_game_allocator.h"

#include <chrono>
#include <cstddef>
#include <string>

namespace kfc {

class IGameServerRegistry;

class RemoteGameAllocator final : public IGameAllocator {
public:
    RemoteGameAllocator(IGameServerRegistry& game_server_registry, std::string service_token,
                        std::chrono::milliseconds timeout, std::size_t max_retry_count);

    GameCreationResponse allocate_game(const GameCreationRequest& request) override;

private:
    IGameServerRegistry& game_server_registry_;
    std::string service_token_;
    std::chrono::milliseconds timeout_;
    std::size_t max_retry_count_;
};

}  // namespace kfc
