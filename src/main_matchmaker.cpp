#include "app/config_loader.h"
#include "app/health_http_server.h"
#include "app/matchmaker_builder.h"

#include <atomic>
#include <csignal>
#include <iostream>

namespace {

std::atomic<kfc::matchmaking::MatchmakerRuntime*> g_matchmaker_for_shutdown{nullptr};

void handle_shutdown_signal(int) {
    if (kfc::matchmaking::MatchmakerRuntime* runtime = g_matchmaker_for_shutdown.load()) {
        runtime->request_stop();
    }
}

void install_shutdown_signal_handlers(kfc::matchmaking::MatchmakerRuntime& runtime) {
    g_matchmaker_for_shutdown.store(&runtime);
    std::signal(SIGINT, handle_shutdown_signal);
#ifdef SIGTERM
    std::signal(SIGTERM, handle_shutdown_signal);
#endif
}

}  // namespace

int main() {
    try {
        const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
        auto built = kfc::app::build_matchmaker(config);
        kfc::app::HealthHttpServer health_server(
            config.server.bind_address, config.server.matchmaker_health_port,
            [&built]() { return built.runtime.metrics(); },
            [&built]() { return built.runtime.is_ready(); });
        health_server.start();
        install_shutdown_signal_handlers(built.runtime);
        built.runtime.run();
        g_matchmaker_for_shutdown.store(nullptr);
        health_server.stop();
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
