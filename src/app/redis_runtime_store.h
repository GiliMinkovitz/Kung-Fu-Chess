#pragma once

#include "app/i_runtime_store.h"
#include "app/redis_config.h"

#include <memory>
#include <string>

namespace kfc {

class RedisRuntimeStore final : public IRuntimeStore {
public:
    RedisRuntimeStore(const app::RedisConfig& config, std::string game_server_endpoint);
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
    [[nodiscard]] std::optional<GameServerLocation> find_player_location(
        UserId user_id) const override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    std::string game_server_endpoint_;
};

}  // namespace kfc
