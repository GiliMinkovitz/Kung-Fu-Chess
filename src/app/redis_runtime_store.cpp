#include "app/redis_runtime_store.h"

#include <hiredis/hiredis.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace kfc {

namespace {

void free_redis_reply(redisReply* reply) {
    if (reply != nullptr) {
        freeReplyObject(reply);
    }
}

bool reply_is_ok(const redisReply* reply) {
    return reply != nullptr && reply->type == REDIS_REPLY_STATUS &&
           std::string_view{reply->str, reply->len} == "OK";
}

bool reply_is_pong(const redisReply* reply) {
    return reply != nullptr && reply->type == REDIS_REPLY_STATUS &&
           std::string_view{reply->str, reply->len} == "PONG";
}

std::string room_key(RoomId room_id) {
    return "room:" + std::to_string(room_id);
}

std::string player_key(UserId player_id) {
    return "player:" + std::to_string(player_id);
}

std::string gameserver_key(std::string_view server_id) {
    return "gameserver:" + std::string(server_id);
}

}  // namespace

struct RedisRuntimeStore::Impl {
    redisContext* context = nullptr;

    explicit Impl(const app::RedisConfig& config) {
        context = redisConnect(config.host.c_str(), config.port);
        if (context == nullptr) {
            return;
        }
        if (context->err != 0) {
            redisFree(context);
            context = nullptr;
            return;
        }

        if (!config.password.empty()) {
            redisReply* auth_reply =
                static_cast<redisReply*>(redisCommand(context, "AUTH %s", config.password.c_str()));
            if (!reply_is_ok(auth_reply)) {
                free_redis_reply(auth_reply);
                redisFree(context);
                context = nullptr;
                return;
            }
            free_redis_reply(auth_reply);
        }

        if (config.database != 0) {
            redisReply* select_reply =
                static_cast<redisReply*>(redisCommand(context, "SELECT %d", config.database));
            if (!reply_is_ok(select_reply)) {
                free_redis_reply(select_reply);
                redisFree(context);
                context = nullptr;
                return;
            }
            free_redis_reply(select_reply);
        }
    }

    ~Impl() {
        if (context != nullptr) {
            redisFree(context);
        }
    }

    [[nodiscard]] bool ping() const {
        if (context == nullptr) {
            return false;
        }

        redisReply* reply = static_cast<redisReply*>(redisCommand(context, "PING"));
        const bool available = reply_is_pong(reply);
        free_redis_reply(reply);
        return available;
    }

    void set_key(std::string_view key, std::string_view value) {
        if (context == nullptr) {
            return;
        }

        redisReply* reply =
            static_cast<redisReply*>(redisCommand(context, "SET %b %b", key.data(), key.size(),
                                                  value.data(), value.size()));
        free_redis_reply(reply);
    }

    void delete_key(std::string_view key) {
        if (context == nullptr) {
            return;
        }

        redisReply* reply =
            static_cast<redisReply*>(redisCommand(context, "DEL %b", key.data(), key.size()));
        free_redis_reply(reply);
    }
};

RedisRuntimeStore::RedisRuntimeStore(const app::RedisConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

RedisRuntimeStore::~RedisRuntimeStore() = default;

bool RedisRuntimeStore::is_available() const {
    return impl_ != nullptr && impl_->ping();
}

void RedisRuntimeStore::register_room(const RoomId room_id, const UserId white_player_id,
                                      const UserId black_player_id, const std::string_view server_id) {
    const std::string room_id_value = std::to_string(room_id);
    impl_->set_key(room_key(room_id), server_id);
    impl_->set_key(player_key(white_player_id), room_id_value);
    impl_->set_key(player_key(black_player_id), room_id_value);
}

void RedisRuntimeStore::unregister_room(const RoomId room_id, const UserId white_player_id,
                                        const UserId black_player_id) {
    impl_->delete_key(room_key(room_id));
    impl_->delete_key(player_key(white_player_id));
    impl_->delete_key(player_key(black_player_id));
}

void RedisRuntimeStore::publish_server_heartbeat(const std::string_view server_id,
                                                 const std::string_view region,
                                                 const app::ServerMetrics& metrics) {
    const std::string value = std::string(region) + "|" + std::to_string(metrics.active_rooms) + "|" +
                              std::to_string(metrics.connected_sessions) + "|" +
                              std::to_string(metrics.matchmaking_queue);
    impl_->set_key(gameserver_key(server_id), value);
}

void RedisRuntimeStore::deregister_server(const std::string_view server_id) {
    impl_->delete_key(gameserver_key(server_id));
}

}  // namespace kfc
