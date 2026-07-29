#include "app/observability/observability.h"

namespace kfc::app::observability {

namespace {

NoOpTracer g_tracer;

}  // namespace

void configure_observability(const std::string service, const std::string server_id) {
    logger().configure(service, server_id);
}

ITracer& tracer() {
    return g_tracer;
}

}  // namespace kfc::app::observability
