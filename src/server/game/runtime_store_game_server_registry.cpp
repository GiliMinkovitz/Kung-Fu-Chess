#include "server/game/runtime_store_game_server_registry.h"

#include "app/i_runtime_store.h"

#include <algorithm>
#include <chrono>

namespace kfc {

RuntimeStoreGameServerRegistry::RuntimeStoreGameServerRegistry(
    IRuntimeStore& runtime_store, const std::chrono::seconds game_server_ttl)
    : runtime_store_(runtime_store), game_server_ttl_(game_server_ttl) {}

std::vector<GameServerRecord> RuntimeStoreGameServerRegistry::list_available_servers() const {
    std::vector<GameServerRecord> servers;
    for (GameServerRecord server : runtime_store_.list_game_servers()) {
        if (!is_live_server(server)) {
            continue;
        }
        servers.push_back(std::move(server));
    }

    std::sort(servers.begin(), servers.end(),
              [](const GameServerRecord& lhs, const GameServerRecord& rhs) {
                  if (lhs.active_rooms != rhs.active_rooms) {
                      return lhs.active_rooms < rhs.active_rooms;
                  }
                  return lhs.server_id < rhs.server_id;
              });
    return servers;
}

std::optional<GameServerRecord> RuntimeStoreGameServerRegistry::get_server(
    const std::string_view server_id) const {
    const std::optional<GameServerRecord> server = runtime_store_.get_game_server(server_id);
    if (!server.has_value() || !is_live_server(*server)) {
        return std::nullopt;
    }
    return server;
}

bool RuntimeStoreGameServerRegistry::is_live_server(const GameServerRecord& server) const {
    if (server.status != "healthy") {
        return false;
    }
    if (server.allocation_endpoint.empty()) {
        return false;
    }
    if (server.last_heartbeat <= 0) {
        return false;
    }

    const std::int64_t now = current_epoch_seconds();
    return now - server.last_heartbeat <= game_server_ttl_.count();
}

std::int64_t RuntimeStoreGameServerRegistry::current_epoch_seconds() {
    return std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

}  // namespace kfc
