#include "app/runtime_diagnostics.h"

#include "app/logging_config.h"

namespace kfc::app {

namespace {

bool g_diagnostics_enabled = true;

}  // namespace

void configure_logging(const LoggingConfig& logging) {
    g_diagnostics_enabled = logging.diagnostics_enabled;
}

bool diagnostics_enabled() noexcept {
    return g_diagnostics_enabled;
}

}  // namespace kfc::app
