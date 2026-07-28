#include "app/config_loader.h"

namespace kfc::app {

AppConfig load_config_from_environment() {
    AppConfig config = make_default_config();
    // TODO: apply environment variable overrides in a later phase.
    return config;
}

}  // namespace kfc::app
