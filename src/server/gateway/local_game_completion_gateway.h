#pragma once

#include "server/gateway/i_game_completion_gateway.h"

namespace kfc {

class ClientSessionManager;
class IMessageSink;
class SessionRegistry;

class LocalGameCompletionGateway : public IGameCompletionGateway {
public:
    LocalGameCompletionGateway(SessionRegistry& session_registry,
                               ClientSessionManager& session_manager, IMessageSink& message_sink);

    void notify_game_finished(PlayerId white, PlayerId black, PieceColor winner_color,
                              FinishReason reason, const RatingChange& rating_change) override;
    void cleanup_finished_players(const FinishedPlayerState& white,
                                  const FinishedPlayerState& black) override;

private:
    [[nodiscard]] bool is_player_connected(PlayerId player_id) const;
    void cleanup_finished_player(const FinishedPlayerState& player);

    SessionRegistry& session_registry_;
    ClientSessionManager& session_manager_;
    IMessageSink& message_sink_;
};

}  // namespace kfc
