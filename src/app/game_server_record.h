#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace kfc {

struct GameServerRecord {
    std::string server_id;
    std::string endpoint;
    std::string allocation_endpoint;
    std::string region;
    std::size_t active_rooms = 0;
    std::string status;
    std::int64_t last_heartbeat = 0;
};

}  // namespace kfc
