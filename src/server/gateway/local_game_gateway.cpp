#include "server/gateway/local_game_gateway.h"

#include "server/network/i_message_sink.h"

namespace kfc {

LocalGameGateway::LocalGameGateway(IMessageSink& message_sink) : message_sink_(message_sink) {}

void LocalGameGateway::notify_match_found(PlayerId white, PlayerId black) {
    message_sink_.send(white, "match_found white");
    message_sink_.send(black, "match_found black");
}

void LocalGameGateway::notify_game_start(PlayerId white, PlayerId black) {
    message_sink_.send(white, "game_start white");
    message_sink_.send(black, "game_start black");
}

void LocalGameGateway::notify_search_timeout(PlayerId player) {
    message_sink_.send(player, "search_timeout");
}

}  // namespace kfc
