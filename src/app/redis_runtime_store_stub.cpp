#include "app/redis_runtime_store.h"

#include <utility>

namespace kfc {

struct RedisRuntimeStore::Impl {};

RedisRuntimeStore::RedisRuntimeStore(const app::RedisConfig&) : impl_(std::make_unique<Impl>()) {}

RedisRuntimeStore::~RedisRuntimeStore() = default;

bool RedisRuntimeStore::is_available() const {
    return false;
}

void RedisRuntimeStore::register_room(RoomId, UserId, UserId, std::string_view) {
    // TODO(phase-11.4): Implement Redis SET for room:{id} and player:{id} mappings when hiredis is available.
}

void RedisRuntimeStore::unregister_room(RoomId, UserId, UserId) {
    // TODO(phase-11.4): Implement Redis DEL for room:{id} and player:{id} mappings when hiredis is available.
}

void RedisRuntimeStore::publish_server_heartbeat(std::string_view, std::string_view,
                                                   const app::ServerMetrics&) {
    // TODO(phase-11.4): Implement Redis SET for gameserver:{id} heartbeat payload when hiredis is available.
}

void RedisRuntimeStore::deregister_server(std::string_view) {
    // TODO(phase-11.4): Implement Redis DEL for gameserver:{id} when hiredis is available.
}

}  // namespace kfc
