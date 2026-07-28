#include "app/config_loader.h"
#include "app/database_config.h"
#include "app/health_http_server.h"
#include "app/server_builder.h"
#include "model/board_model.h"
#include "server/game_server.h"

#include <atomic>
#include <csignal>
#include <iostream>

namespace {

std::atomic<kfc::GameServer*> g_server_for_shutdown{nullptr};

void handle_shutdown_signal(int) {
    if (kfc::GameServer* server = g_server_for_shutdown.load()) {
        server->request_stop();
    }
}

void install_shutdown_signal_handlers(kfc::GameServer& server) {
    g_server_for_shutdown.store(&server);
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
        auto built = kfc::app::build_game_server(config, default_board());
        kfc::app::HealthHttpServer health_server(
            config.server.bind_address, config.server.health_port,
            [&built]() { return built.server.metrics(); });
        health_server.start();
        install_shutdown_signal_handlers(built.server);
        built.server.run();
        g_server_for_shutdown.store(nullptr);
        health_server.stop();
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
