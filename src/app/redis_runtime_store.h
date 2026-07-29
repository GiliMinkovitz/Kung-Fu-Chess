#pragma once

#include "app/i_runtime_store.h"
#include "app/redis_config.h"

#include <memory>

namespace kfc {

class RedisRuntimeStore final : public IRuntimeStore {
public:
    explicit RedisRuntimeStore(const app::RedisConfig& config);
    ~RedisRuntimeStore() override;

    RedisRuntimeStore(const RedisRuntimeStore&) = delete;
    RedisRuntimeStore& operator=(const RedisRuntimeStore&) = delete;

    [[nodiscard]] bool is_available() const override;
    void register_room(RoomId room_id, UserId white_player_id, UserId black_player_id,
                       std::string_view server_id) override;
    void unregister_room(RoomId room_id, UserId white_player_id, UserId black_player_id) override;
    void publish_server_heartbeat(std::string_view server_id, std::string_view region,
                                  const app::ServerMetrics& metrics) override;
    void deregister_server(std::string_view server_id) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace kfc
