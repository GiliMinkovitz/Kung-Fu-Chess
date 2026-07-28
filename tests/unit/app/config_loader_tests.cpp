#include "app/config_loader.h"
#include "app/server_config.h"

#include <doctest/doctest.h>

#include <cstdlib>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

#ifdef _WIN32
void set_environment(const char* name, const char* value) {
    _putenv_s(name, value);
}

void clear_environment(const char* name) {
    _putenv_s(name, "");
}
#else
void set_environment(const char* name, const char* value) {
    setenv(name, value, 1);
}

void clear_environment(const char* name) {
    unsetenv(name);
}
#endif

class ScopedEnvironment {
public:
    explicit ScopedEnvironment(std::initializer_list<std::pair<const char*, std::optional<const char*>>> overrides) {
        for (const auto& [name, value] : overrides) {
            save(name);
            if (value.has_value()) {
                set_environment(name, *value);
            } else {
                clear_environment(name);
            }
        }
    }

    ~ScopedEnvironment() {
        for (auto it = saved_.rbegin(); it != saved_.rend(); ++it) {
            if (it->second.has_value()) {
                set_environment(it->first.c_str(), it->second->c_str());
            } else {
                clear_environment(it->first.c_str());
            }
        }
    }

    ScopedEnvironment(const ScopedEnvironment&) = delete;
    ScopedEnvironment& operator=(const ScopedEnvironment&) = delete;

private:
    void save(const char* name) {
        if (const char* value = std::getenv(name)) {
            saved_.emplace_back(name, std::string(value));
        } else {
            saved_.emplace_back(name, std::nullopt);
        }
    }

    std::vector<std::pair<std::string, std::optional<std::string>>> saved_;
};

void check_default_server_config(const kfc::app::ServerConfig& server) {
    CHECK_EQ(server.port, 8765);
    CHECK_EQ(server.bind_address, "127.0.0.1");
    CHECK_EQ(server.max_clients, kfc::app::ServerConfig::kDefaultMaxClients);
}

}  // namespace

TEST_CASE("ConfigLoaderTest - DefaultConfigurationWithoutEnvironment") {
    const ScopedEnvironment env{
        {"KFC_PORT", std::nullopt},
        {"KFC_BIND_ADDRESS", std::nullopt},
        {"KFC_MAX_CLIENTS", std::nullopt},
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    check_default_server_config(config.server);
}

TEST_CASE("ConfigLoaderTest - OverridesPort") {
    const ScopedEnvironment env{
        {"KFC_PORT", "9000"},
        {"KFC_BIND_ADDRESS", std::nullopt},
        {"KFC_MAX_CLIENTS", std::nullopt},
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    CHECK_EQ(config.server.port, 9000);
    CHECK_EQ(config.server.bind_address, "127.0.0.1");
    CHECK_EQ(config.server.max_clients, kfc::app::ServerConfig::kDefaultMaxClients);
}

TEST_CASE("ConfigLoaderTest - OverridesBindAddress") {
    const ScopedEnvironment env{
        {"KFC_PORT", std::nullopt},
        {"KFC_BIND_ADDRESS", "0.0.0.0"},
        {"KFC_MAX_CLIENTS", std::nullopt},
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    CHECK_EQ(config.server.port, 8765);
    CHECK_EQ(config.server.bind_address, "0.0.0.0");
    CHECK_EQ(config.server.max_clients, kfc::app::ServerConfig::kDefaultMaxClients);
}

TEST_CASE("ConfigLoaderTest - OverridesMaxClients") {
    const ScopedEnvironment env{
        {"KFC_PORT", std::nullopt},
        {"KFC_BIND_ADDRESS", std::nullopt},
        {"KFC_MAX_CLIENTS", "64"},
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    CHECK_EQ(config.server.port, 8765);
    CHECK_EQ(config.server.bind_address, "127.0.0.1");
    CHECK_EQ(config.server.max_clients, 64);
}

TEST_CASE("ConfigLoaderTest - AppliesMultipleOverridesTogether") {
    const ScopedEnvironment env{
        {"KFC_PORT", "9000"},
        {"KFC_BIND_ADDRESS", "0.0.0.0"},
        {"KFC_MAX_CLIENTS", "64"},
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    CHECK_EQ(config.server.port, 9000);
    CHECK_EQ(config.server.bind_address, "0.0.0.0");
    CHECK_EQ(config.server.max_clients, 64);
}

TEST_CASE("ConfigLoaderTest - IgnoresInvalidNumericValues") {
    const ScopedEnvironment env{
        {"KFC_PORT", "abc"},
        {"KFC_BIND_ADDRESS", std::nullopt},
        {"KFC_MAX_CLIENTS", "not-a-number"},
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    check_default_server_config(config.server);
}

TEST_CASE("ConfigLoaderTest - IgnoresZeroAndOutOfRangePort") {
    SUBCASE("ZeroPort") {
        const ScopedEnvironment env{
            {"KFC_PORT", "0"},
            {"KFC_BIND_ADDRESS", std::nullopt},
            {"KFC_MAX_CLIENTS", std::nullopt},
        };
        const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
        CHECK_EQ(config.server.port, 8765);
    }

    SUBCASE("OutOfRangePort") {
        const ScopedEnvironment env{
            {"KFC_PORT", "70000"},
            {"KFC_BIND_ADDRESS", std::nullopt},
            {"KFC_MAX_CLIENTS", std::nullopt},
        };
        const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
        CHECK_EQ(config.server.port, 8765);
    }
}

TEST_CASE("ConfigLoaderTest - IgnoresZeroMaxClients") {
    const ScopedEnvironment env{
        {"KFC_PORT", std::nullopt},
        {"KFC_BIND_ADDRESS", std::nullopt},
        {"KFC_MAX_CLIENTS", "0"},
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    CHECK_EQ(config.server.max_clients, kfc::app::ServerConfig::kDefaultMaxClients);
}

TEST_CASE("ConfigLoaderTest - IgnoresEmptyBindAddress") {
    const ScopedEnvironment env{
        {"KFC_PORT", std::nullopt},
        {"KFC_BIND_ADDRESS", ""},
        {"KFC_MAX_CLIENTS", std::nullopt},
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    CHECK_EQ(config.server.bind_address, "127.0.0.1");
}

TEST_CASE("ConfigLoaderTest - MissingVariablesKeepDefaults") {
    const ScopedEnvironment env{
        {"KFC_PORT", std::nullopt},
        {"KFC_BIND_ADDRESS", std::nullopt},
        {"KFC_MAX_CLIENTS", std::nullopt},
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    check_default_server_config(config.server);
}
