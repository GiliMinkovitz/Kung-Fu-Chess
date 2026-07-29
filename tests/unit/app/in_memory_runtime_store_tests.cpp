#include "app/in_memory_runtime_store.h"
#include "app/server_metrics.h"

#include <doctest/doctest.h>

TEST_CASE("InMemoryRuntimeStoreTest - FindPlayerLocationAfterRegister") {
    kfc::InMemoryRuntimeStore store;
    store.register_room(42, 1, 2, "server-a", "ws://localhost:8765");

    const std::optional<kfc::GameServerLocation> white_location = store.find_player_location(1);
    REQUIRE(white_location.has_value());
    CHECK_EQ(white_location->room_id, 42u);
    CHECK_EQ(white_location->server_id, "server-a");
    CHECK_EQ(white_location->endpoint, "ws://localhost:8765");

    const std::optional<kfc::GameServerLocation> black_location = store.find_player_location(2);
    REQUIRE(black_location.has_value());
    CHECK_EQ(black_location->room_id, 42u);
    CHECK_EQ(black_location->server_id, "server-a");
    CHECK_EQ(black_location->endpoint, "ws://localhost:8765");
}

TEST_CASE("InMemoryRuntimeStoreTest - FindPlayerLocationMissingAfterUnregister") {
    kfc::InMemoryRuntimeStore store;
    store.register_room(7, 10, 11, "server-b", "ws://localhost:8765");
    store.unregister_room(7, 10, 11);

    CHECK_FALSE(store.find_player_location(10).has_value());
    CHECK_FALSE(store.find_player_location(11).has_value());
}

TEST_CASE("InMemoryRuntimeStoreTest - UsesExplicitEndpointWithoutFallback") {
    kfc::InMemoryRuntimeStore store;
    store.register_room(99, 3, 4, "server-c", "ws://explicit.example:9000");

    kfc::app::ServerMetrics metrics;
    metrics.endpoint = "ws://games.example:9000";
    store.publish_server_heartbeat("server-c", "eu-west", metrics);

    const std::optional<kfc::GameServerLocation> location = store.find_player_location(3);
    REQUIRE(location.has_value());
    CHECK_EQ(location->room_id, 99u);
    CHECK_EQ(location->server_id, "server-c");
    CHECK_EQ(location->endpoint, "ws://explicit.example:9000");
}

TEST_CASE("InMemoryRuntimeStoreTest - ListGameServersFromHeartbeat") {
    kfc::InMemoryRuntimeStore store;
    kfc::app::ServerMetrics metrics;
    metrics.endpoint = "ws://games.example:9000";
    metrics.allocation_endpoint = "http://games.example:9001";
    metrics.active_rooms = 3;
    store.publish_server_heartbeat("server-d", "us-east", metrics);

    const std::vector<kfc::GameServerRecord> servers = store.list_game_servers();
    REQUIRE_EQ(servers.size(), 1u);
    CHECK_EQ(servers[0].server_id, "server-d");
    CHECK_EQ(servers[0].endpoint, "ws://games.example:9000");
    CHECK_EQ(servers[0].allocation_endpoint, "http://games.example:9001");
    CHECK_EQ(servers[0].active_rooms, 3u);
    CHECK_EQ(servers[0].status, "healthy");
    CHECK_GT(servers[0].last_heartbeat, 0);
}

TEST_CASE("InMemoryRuntimeStoreTest - FindPlayerLocationReturnsEmptyForUnknownPlayer") {
    kfc::InMemoryRuntimeStore store;
    CHECK_FALSE(store.find_player_location(404).has_value());
}
