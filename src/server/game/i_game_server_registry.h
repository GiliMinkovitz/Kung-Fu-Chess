#pragma once

#include "app/game_server_record.h"

#include <optional>
#include <string_view>
#include <vector>

namespace kfc {

class IGameServerRegistry {
public:
    virtual ~IGameServerRegistry() = default;

    [[nodiscard]] virtual std::vector<GameServerRecord> list_available_servers() const = 0;
    [[nodiscard]] virtual std::optional<GameServerRecord> get_server(
        std::string_view server_id) const = 0;
};

}  // namespace kfc
