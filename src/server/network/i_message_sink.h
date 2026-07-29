#pragma once

#include "server/network/player_id.h"

#include <string_view>

namespace kfc {

class IMessageSink {
public:
    virtual ~IMessageSink() = default;

    virtual bool send(PlayerId player, std::string_view message) = 0;
    virtual bool send_message(PlayerId player, std::string_view message) = 0;
};

}  // namespace kfc
