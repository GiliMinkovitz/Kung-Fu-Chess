#include "app/redis_runtime_store.h"

#include <hiredis/hiredis.h>

#include <cstdint>
#include <optional>
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

std::string encode_player_location_value(RoomId room_id, std::string_view server_id,
                                         std::string_view endpoint) {
    return std::to_string(room_id) + "|" + std::string(server_id) + "|" + std::string(endpoint);
}

bool parse_player_location_value(std::string_view value, RoomId& room_id, std::string& server_id,
                                 std::string& endpoint) {
    const std::size_t first = value.find('|');
    if (first == std::string_view::npos) {
        return false;
    }

    const std::size_t second = value.find('|', first + 1);
    if (second == std::string_view::npos) {
        return false;
    }

    try {
        room_id = static_cast<RoomId>(std::stoull(std::string(value.substr(0, first))));
    } catch (...) {
        return false;
    }

    server_id = std::string(value.substr(first + 1, second - first - 1));
    endpoint = std::string(value.substr(second + 1));
    return true;
}

std::optional<std::string> parse_gameserver_endpoint(std::string_view value) {
    std::size_t delimiter_count = 0;
    std::size_t last_delimiter = std::string_view::npos;
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] != '|') {
            continue;
        }
        ++delimiter_count;
        last_delimiter = i;
    }

    if (delimiter_count < 4 || last_delimiter == std::string_view::npos ||
        last_delimiter + 1 >= value.size()) {
        return std::nullopt;
    }

    return std::string(value.substr(last_delimiter + 1));
}

}  // namespace

struct RedisRuntimeStore::Impl {
    mutable redisContext* context = nullptr;

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

    [[nodiscard]] std::optional<std::string> get_key(std::string_view key) const {
        if (context == nullptr) {
            return std::nullopt;
        }

        redisReply* reply =
            static_cast<redisReply*>(redisCommand(context, "GET %b", key.data(), key.size()));
        if (reply == nullptr || reply->type != REDIS_REPLY_STRING) {
            free_redis_reply(reply);
            return std::nullopt;
        }

        const std::string value(reply->str, reply->len);
        free_redis_reply(reply);
        return value;
    }
};

RedisRuntimeStore::RedisRuntimeStore(const app::RedisConfig& config,
                                     std::string game_server_endpoint)
    : impl_(std::make_unique<Impl>(config)),
      game_server_endpoint_(std::move(game_server_endpoint)) {}

RedisRuntimeStore::~RedisRuntimeStore() = default;

bool RedisRuntimeStore::is_available() const {
    return impl_ != nullptr && impl_->ping();
}

void RedisRuntimeStore::register_room(const RoomId room_id, const UserId white_player_id,
                                      const UserId black_player_id, const std::string_view server_id) {
    const std::string player_location =
        encode_player_location_value(room_id, server_id, game_server_endpoint_);
    impl_->set_key(room_key(room_id), server_id);
    impl_->set_key(player_key(white_player_id), player_location);
    impl_->set_key(player_key(black_player_id), player_location);
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
                              std::to_string(metrics.matchmaking_queue) + "|" + metrics.endpoint;
    impl_->set_key(gameserver_key(server_id), value);
}

void RedisRuntimeStore::deregister_server(const std::string_view server_id) {
    impl_->delete_key(gameserver_key(server_id));
}

std::optional<GameServerLocation> RedisRuntimeStore::find_player_location(
    const UserId user_id) const {
    const std::optional<std::string> stored = impl_->get_key(player_key(user_id));
    if (!stored.has_value()) {
        return std::nullopt;
    }

    RoomId room_id = 0;
    std::string server_id;
    std::string endpoint;
    if (!parse_player_location_value(*stored, room_id, server_id, endpoint)) {
        return std::nullopt;
    }

    if (endpoint.empty()) {
        if (const std::optional<std::string> gameserver_value = impl_->get_key(gameserver_key(server_id))) {
            if (const std::optional<std::string> heartbeat_endpoint =
                    parse_gameserver_endpoint(*gameserver_value)) {
                endpoint = *heartbeat_endpoint;
            }
        }
    }

    return GameServerLocation{server_id, endpoint, room_id};
}

}  // namespace kfc
