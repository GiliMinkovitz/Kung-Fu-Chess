#include "app/config_loader.h"
#include "app/database_config.h"
#include "app/health_http_server.h"
#include "app/runtime_endpoint.h"
#include "app/observability/metric_counters.h"
#include "app/observability/observability.h"
#include "app/observability/prometheus_formatter.h"
#include "app/observability/readiness_checker.h"
#include "app/server_builder.h"

#include <atomic>
#include <csignal>
#include <iostream>

namespace {

std::atomic<kfc::GatewayServer*> g_gateway_for_shutdown{nullptr};

void handle_shutdown_signal(int) {
    if (kfc::GatewayServer* gateway = g_gateway_for_shutdown.load()) {
        gateway->request_stop();
    }
}

void install_shutdown_signal_handlers(kfc::GatewayServer& gateway) {
    g_gateway_for_shutdown.store(&gateway);
    std::signal(SIGINT, handle_shutdown_signal);
#ifdef SIGTERM
    std::signal(SIGTERM, handle_shutdown_signal);
#endif
}

}  // namespace

int main() {
    try {
        const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
        kfc::app::observability::configure_observability("gateway",
                                                         config.server.gateway_server_id);
        std::cout << "Database backend: "
                  << kfc::app::database_backend_name(config.database.backend) << '\n';
        auto built = kfc::app::build_gateway(config);
        kfc::app::HealthHttpServer health_server(
            config.server.bind_address, config.server.health_port,
            [&built]() {
                return kfc::app::observability::format_prometheus_metrics(
                    kfc::app::observability::ServiceKind::Gateway, built.gateway.metrics(),
                    kfc::app::observability::metrics(),
                    built.gateway.authenticated_player_count());
            },
            [&built, &config]() {
                return kfc::app::observability::check_gateway_ready(
                    built.infrastructure.database(), config.redis,
                    built.infrastructure.runtime_store(),
                    kfc::app::resolve_matchmaker_health_endpoint(config.server));
            });
        health_server.start();
        install_shutdown_signal_handlers(built.gateway);
        built.gateway.run();
        g_gateway_for_shutdown.store(nullptr);
        health_server.stop();
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
