#pragma once

#include "server/game/i_game_server_registry.h"

#include <chrono>

namespace kfc {

class IRuntimeStore;

class RuntimeStoreGameServerRegistry final : public IGameServerRegistry {
public:
    RuntimeStoreGameServerRegistry(IRuntimeStore& runtime_store,
                                   std::chrono::seconds game_server_ttl);

    [[nodiscard]] std::vector<GameServerRecord> list_available_servers() const override;
    [[nodiscard]] std::optional<GameServerRecord> get_server(
        std::string_view server_id) const override;

private:
    [[nodiscard]] bool is_live_server(const GameServerRecord& server) const;
    [[nodiscard]] static std::int64_t current_epoch_seconds();

    IRuntimeStore& runtime_store_;
    std::chrono::seconds game_server_ttl_;
};

}  // namespace kfc
