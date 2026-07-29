#include "app/config_loader.h"
#include "app/database_config.h"
#include "app/health_http_server.h"
#include "app/observability/metric_counters.h"
#include "app/observability/observability.h"
#include "app/observability/prometheus_formatter.h"
#include "app/observability/readiness_checker.h"
#include "app/server_builder.h"
#include "model/board_model.h"

#include <atomic>
#include <csignal>
#include <iostream>

namespace {

std::atomic<kfc::GameServerRuntime*> g_runtime_for_shutdown{nullptr};

void handle_shutdown_signal(int) {
    if (kfc::GameServerRuntime* runtime = g_runtime_for_shutdown.load()) {
        runtime->request_stop();
    }
}

void install_shutdown_signal_handlers(kfc::GameServerRuntime& runtime) {
    g_runtime_for_shutdown.store(&runtime);
    std::signal(SIGINT, handle_shutdown_signal);
#ifdef SIGTERM
    std::signal(SIGTERM, handle_shutdown_signal);
#endif
}

kfc::BoardModel default_board() {
    return kfc::BoardModel::from_token_grid({
        {"bR", "bN", "bB", "bQ", "bK", "bB", "bN", "bR"},
        {"bP", "bP", "bP", "bP", "bP", "bP", "bP", "bP"},
        {".", ".", ".", ".", ".", ".", ".", "."},
        {".", ".", ".", ".", ".", ".", ".", "."},
        {".", ".", ".", ".", ".", ".", ".", "."},
        {".", ".", ".", ".", ".", ".", ".", "."},
        {"wP", "wP", "wP", "wP", "wP", "wP", "wP", "wP"},
        {"wR", "wN", "wB", "wQ", "wK", "wB", "wN", "wR"},
    });
}

}  // namespace

int main() {
    try {
        const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
        kfc::app::observability::configure_observability("game-server", config.server.server_id);
        std::cout << "Database backend: "
                  << kfc::app::database_backend_name(config.database.backend) << '\n';
        auto built = kfc::app::build_game_server_runtime(config, default_board());
        kfc::app::HealthHttpServer health_server(
            config.server.bind_address, config.server.game_health_port,
            [&built]() {
                return kfc::app::observability::format_prometheus_metrics(
                    kfc::app::observability::ServiceKind::GameServer, built.runtime.metrics(),
                    kfc::app::observability::metrics(), 0, built.runtime.active_player_count(), 0,
                    built.runtime.is_allocation_api_active());
            },
            [&built, &config]() {
                return kfc::app::observability::check_game_server_ready(
                    built.infrastructure.database(), config.redis,
                    built.infrastructure.runtime_store(),
                    built.runtime.is_allocation_api_active());
            });
        health_server.start();
        install_shutdown_signal_handlers(built.runtime);
        built.runtime.run();
        g_runtime_for_shutdown.store(nullptr);
        health_server.stop();
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
