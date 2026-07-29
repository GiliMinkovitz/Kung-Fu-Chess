#include "app/config_loader.h"
#include "app/database_config.h"
#include "app/health_http_server.h"
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
        std::cout << "Database backend: "
                  << kfc::app::database_backend_name(config.database.backend) << '\n';
        auto built = kfc::app::build_game_server_runtime(config, default_board());
        kfc::app::HealthHttpServer health_server(
            config.server.bind_address, config.server.game_health_port,
            [&built]() { return built.runtime.metrics(); },
            [&built, &config]() {
                if (!config.redis.enabled) {
                    return true;
                }
                return built.infrastructure.runtime_store().is_available();
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
