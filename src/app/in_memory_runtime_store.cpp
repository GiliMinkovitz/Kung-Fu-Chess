#include "app/in_memory_runtime_store.h"

namespace kfc {

bool InMemoryRuntimeStore::is_available() const {
    return false;
}

void InMemoryRuntimeStore::register_room(RoomId, UserId, UserId, std::string_view) {}

void InMemoryRuntimeStore::unregister_room(RoomId, UserId, UserId) {}

void InMemoryRuntimeStore::publish_server_heartbeat(std::string_view, std::string_view,
                                                    const app::ServerMetrics&) {}

void InMemoryRuntimeStore::deregister_server(std::string_view) {}

}  // namespace kfc
