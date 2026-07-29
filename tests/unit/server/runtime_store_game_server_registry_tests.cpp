#include "app/in_memory_runtime_store.h"
#include "app/server_metrics.h"
#include "server/game/runtime_store_game_server_registry.h"

#include <doctest/doctest.h>

#include <chrono>
#include <thread>

namespace {

kfc::app::ServerMetrics make_metrics(const std::string& endpoint,
                                     const std::string& allocation_endpoint,
                                     const std::size_t active_rooms = 0) {
    kfc::app::ServerMetrics metrics;
    metrics.endpoint = endpoint;
    metrics.allocation_endpoint = allocation_endpoint;
    metrics.active_rooms = active_rooms;
    return metrics;
}

}  // namespace

TEST_CASE("RuntimeStoreGameServerRegistryTest - ListsHealthyServersSortedByLoad") {
    kfc::InMemoryRuntimeStore store;
    store.publish_server_heartbeat("server-b", "local",
                                   make_metrics("ws://b:1", "http://b:2", 5));
    store.publish_server_heartbeat("server-a", "local",
                                   make_metrics("ws://a:1", "http://a:2", 1));

    const kfc::RuntimeStoreGameServerRegistry registry(store, std::chrono::seconds(10));
    const std::vector<kfc::GameServerRecord> servers = registry.list_available_servers();

    REQUIRE_EQ(servers.size(), 2u);
    CHECK_EQ(servers[0].server_id, "server-a");
    CHECK_EQ(servers[1].server_id, "server-b");
}

TEST_CASE("RuntimeStoreGameServerRegistryTest - IgnoresExpiredHeartbeats") {
    kfc::InMemoryRuntimeStore store;
    store.publish_server_heartbeat("server-live", "local",
                                   make_metrics("ws://live:1", "http://live:2"));
    store.publish_server_heartbeat("server-stale", "local",
                                   make_metrics("ws://stale:1", "http://stale:2"));

    kfc::RuntimeStoreGameServerRegistry registry(store, std::chrono::seconds(1));
    REQUIRE_EQ(registry.list_available_servers().size(), 2u);

    std::this_thread::sleep_for(std::chrono::milliseconds(2100));

    const std::vector<kfc::GameServerRecord> servers = registry.list_available_servers();
    CHECK(servers.empty());
}

TEST_CASE("RuntimeStoreGameServerRegistryTest - IgnoresServersWithoutAllocationEndpoint") {
    kfc::InMemoryRuntimeStore store;
    kfc::app::ServerMetrics metrics = make_metrics("ws://a:1", "");
    store.publish_server_heartbeat("server-a", "local", metrics);

    const kfc::RuntimeStoreGameServerRegistry registry(store, std::chrono::seconds(10));
    CHECK(registry.list_available_servers().empty());
}

TEST_CASE("RuntimeStoreGameServerRegistryTest - GetServerReturnsLiveRecord") {
    kfc::InMemoryRuntimeStore store;
    store.publish_server_heartbeat("server-a", "local",
                                   make_metrics("ws://a:1", "http://a:2", 3));

    const kfc::RuntimeStoreGameServerRegistry registry(store, std::chrono::seconds(10));
    const std::optional<kfc::GameServerRecord> server = registry.get_server("server-a");

    REQUIRE(server.has_value());
    CHECK_EQ(server->server_id, "server-a");
    CHECK_EQ(server->endpoint, "ws://a:1");
    CHECK_EQ(server->allocation_endpoint, "http://a:2");
    CHECK_EQ(server->active_rooms, 3u);
}
