#pragma once

#include "app/app_config.h"

namespace kfc::app {

[[nodiscard]]
AppConfig load_config_from_environment();

}  // namespace kfc::app
