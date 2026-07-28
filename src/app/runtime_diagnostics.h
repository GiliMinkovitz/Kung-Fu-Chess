#pragma once

namespace kfc::app {

struct LoggingConfig;

void configure_logging(const LoggingConfig& logging);
[[nodiscard]] bool diagnostics_enabled() noexcept;

}  // namespace kfc::app
