#include "app/config_loader.h"
#include "app/health_http_server.h"
#include "app/observability/metric_counters.h"
#include "app/observability/observability.h"
#include "app/observability/prometheus_formatter.h"
#include "app/observability/readiness_checker.h"
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
        kfc::app::observability::configure_observability("matchmaker", config.server.server_id);
        auto built = kfc::app::build_matchmaker(config);
        kfc::app::HealthHttpServer health_server(
            config.server.bind_address, config.server.matchmaker_health_port,
            [&built]() {
                return kfc::app::observability::format_prometheus_metrics(
                    kfc::app::observability::ServiceKind::Matchmaker, built.runtime.metrics(),
                    kfc::app::observability::metrics(), 0, 0,
                    built.runtime.active_game_server_count());
            },
            [&built, &config]() {
                return kfc::app::observability::check_matchmaker_ready(
                    built.infrastructure.database(), config.redis,
                    built.infrastructure.runtime_store());
            });
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
