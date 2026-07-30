#include "server/gateway/local_game_gateway.h"

#include "model/piece.h"
#include "server/network/i_message_sink.h"

#include <string>

namespace kfc {

namespace {

const char* side_token(const PieceColor side) {
    return side == PieceColor::White ? "white" : "black";
}

}  // namespace

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

void LocalGameGateway::send_game_redirect(PlayerId player, GatewayGameRedirectInfo redirect_info) {
    std::string message = "game_redirect ";
    message += redirect_info.endpoint;
    message += ' ';
    message += std::to_string(redirect_info.room_id);
    message += ' ';
    message += side_token(redirect_info.side);
    message_sink_.send(player, message);
}

}  // namespace kfc
