#include "app/redis_runtime_store.h"

#include <hiredis/hiredis.h>

#include <chrono>
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

std::int64_t current_epoch_seconds() {
    return std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
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

std::string queue_key(std::string_view region) {
    return "queue:" + std::string(region);
}

std::optional<std::string> field_at(std::string_view value, std::size_t index) {
    std::size_t start = 0;
    for (std::size_t current = 0; current <= index; ++current) {
        const std::size_t next = value.find('|', start);
        if (current == index) {
            if (next == std::string_view::npos) {
                return std::string(value.substr(start));
            }
            return std::string(value.substr(start, next - start));
        }
        if (next == std::string_view::npos) {
            return std::nullopt;
        }
        start = next + 1;
    }
    return std::nullopt;
}

std::string encode_queue_entry(const matchmaking::QueuedPlayer& player) {
    return std::to_string(player.player_id) + "|" + std::to_string(player.user_id) + "|" +
           std::to_string(player.elo) + "|" + std::to_string(player.enqueue_epoch_seconds);
}

std::optional<matchmaking::QueuedPlayer> decode_queue_entry(std::string_view value) {
    const std::optional<std::string> player_id = field_at(value, 0);
    const std::optional<std::string> user_id = field_at(value, 1);
    const std::optional<std::string> elo = field_at(value, 2);
    const std::optional<std::string> timestamp = field_at(value, 3);
    if (!player_id.has_value() || !user_id.has_value() || !elo.has_value() ||
        !timestamp.has_value()) {
        return std::nullopt;
    }

    matchmaking::QueuedPlayer player;
    try {
        player.player_id = static_cast<PlayerId>(std::stoull(*player_id));
        player.user_id = static_cast<UserId>(std::stoull(*user_id));
        player.elo = std::stoi(*elo);
        player.enqueue_epoch_seconds = std::stoll(*timestamp);
    } catch (...) {
        return std::nullopt;
    }
    return player;
}

std::string encode_queue_value(const std::vector<matchmaking::QueuedPlayer>& queue) {
    std::string encoded;
    for (std::size_t i = 0; i < queue.size(); ++i) {
        if (i > 0) {
            encoded.push_back('\n');
        }
        encoded += encode_queue_entry(queue[i]);
    }
    return encoded;
}

std::vector<matchmaking::QueuedPlayer> decode_queue_value(std::string_view value) {
    std::vector<matchmaking::QueuedPlayer> queue;
    std::size_t start = 0;
    while (start < value.size()) {
        const std::size_t end = value.find('\n', start);
        const std::string_view line =
            end == std::string_view::npos ? value.substr(start) : value.substr(start, end - start);
        if (const std::optional<matchmaking::QueuedPlayer> player = decode_queue_entry(line)) {
            queue.push_back(*player);
        }
        if (end == std::string_view::npos) {
            break;
        }
        start = end + 1;
    }
    return queue;
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

std::optional<GameServerRecord> parse_gameserver_record(std::string_view server_id,
                                                        std::string_view value) {
    const std::optional<std::string> region = field_at(value, 0);
    const std::optional<std::string> active_rooms = field_at(value, 1);
    const std::optional<std::string> endpoint = field_at(value, 4);
    if (!region.has_value() || !active_rooms.has_value() || !endpoint.has_value()) {
        return std::nullopt;
    }

    GameServerRecord record;
    record.server_id = std::string(server_id);
    record.region = *region;
    try {
        record.active_rooms = static_cast<std::size_t>(std::stoull(*active_rooms));
    } catch (...) {
        record.active_rooms = 0;
    }
    record.endpoint = *endpoint;
    if (const std::optional<std::string> allocation_endpoint = field_at(value, 5)) {
        record.allocation_endpoint = *allocation_endpoint;
    }
    if (const std::optional<std::string> status = field_at(value, 6)) {
        record.status = *status;
    } else {
        record.status = "healthy";
    }
    if (const std::optional<std::string> heartbeat = field_at(value, 7)) {
        try {
            record.last_heartbeat = std::stoll(*heartbeat);
        } catch (...) {
            record.last_heartbeat = 0;
        }
    }
    return record;
}

std::string encode_gameserver_heartbeat(std::string_view region, const app::ServerMetrics& metrics) {
    return std::string(region) + "|" + std::to_string(metrics.active_rooms) + "|" +
           std::to_string(metrics.connected_sessions) + "|" +
           std::to_string(metrics.matchmaking_queue) + "|" + metrics.endpoint + "|" +
           metrics.allocation_endpoint + "|healthy|" + std::to_string(current_epoch_seconds());
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

    [[nodiscard]] std::vector<std::string> scan_keys(std::string_view pattern) const {
        std::vector<std::string> keys;
        if (context == nullptr) {
            return keys;
        }

        std::string cursor = "0";
        do {
            redisReply* reply = static_cast<redisReply*>(
                redisCommand(context, "SCAN %s MATCH %b COUNT 100", cursor.c_str(), pattern.data(),
                             pattern.size()));
            if (reply == nullptr || reply->type != REDIS_REPLY_ARRAY || reply->elements < 2) {
                free_redis_reply(reply);
                break;
            }

            if (reply->element[0]->type == REDIS_REPLY_STRING) {
                cursor.assign(reply->element[0]->str, reply->element[0]->len);
            }

            redisReply* key_array = reply->element[1];
            if (key_array != nullptr && key_array->type == REDIS_REPLY_ARRAY) {
                for (std::size_t i = 0; i < key_array->elements; ++i) {
                    redisReply* key_reply = key_array->element[i];
                    if (key_reply != nullptr && key_reply->type == REDIS_REPLY_STRING) {
                        keys.emplace_back(key_reply->str, key_reply->len);
                    }
                }
            }

            free_redis_reply(reply);
        } while (cursor != "0");

        return keys;
    }
};

RedisRuntimeStore::RedisRuntimeStore(const app::RedisConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

RedisRuntimeStore::~RedisRuntimeStore() = default;

bool RedisRuntimeStore::is_available() const {
    return impl_ != nullptr && impl_->ping();
}

void RedisRuntimeStore::register_room(const RoomId room_id, const UserId white_player_id,
                                      const UserId black_player_id, const std::string_view server_id,
                                      const std::string_view endpoint) {
    const std::string player_location =
        encode_player_location_value(room_id, server_id, endpoint);
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
    impl_->set_key(gameserver_key(server_id), encode_gameserver_heartbeat(region, metrics));
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

    return GameServerLocation{server_id, endpoint, room_id};
}

std::vector<GameServerRecord> RedisRuntimeStore::list_game_servers() const {
    std::vector<GameServerRecord> servers;
    constexpr std::string_view prefix = "gameserver:";
    for (const std::string& key : impl_->scan_keys("gameserver:*")) {
        if (key.size() <= prefix.size()) {
            continue;
        }
        const std::string server_id = key.substr(prefix.size());
        if (const std::optional<std::string> value = impl_->get_key(key)) {
            if (const std::optional<GameServerRecord> record = parse_gameserver_record(server_id, *value)) {
                servers.push_back(*record);
            }
        }
    }
    return servers;
}

std::optional<GameServerRecord> RedisRuntimeStore::get_game_server(
    const std::string_view server_id) const {
    if (const std::optional<std::string> value = impl_->get_key(gameserver_key(server_id))) {
        return parse_gameserver_record(server_id, *value);
    }
    return std::nullopt;
}

void RedisRuntimeStore::set_matchmaking_queue(const std::string_view region,
                                              const std::vector<matchmaking::QueuedPlayer>& queue) {
    impl_->set_key(queue_key(region), encode_queue_value(queue));
}

std::vector<matchmaking::QueuedPlayer> RedisRuntimeStore::list_matchmaking_queue(
    const std::string_view region) const {
    if (const std::optional<std::string> value = impl_->get_key(queue_key(region))) {
        return decode_queue_value(*value);
    }
    return {};
}

}  // namespace kfc
