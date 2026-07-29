#pragma once

#include "model/piece.h"
#include "server/game_result_message_writer.h"
#include "server/network/player_id.h"
#include "server/rating_service.h"
#include "server/user/user_id.h"

#include <string>

namespace kfc {

struct FinishedPlayerState {
    PlayerId player_id = 0;
    UserId user_id = 0;
    std::string username;
    int rating = 0;
    bool has_user = false;
};

class IGameCompletionGateway {
public:
    virtual ~IGameCompletionGateway() = default;

    virtual void notify_game_finished(PlayerId white, PlayerId black, PieceColor winner_color,
                                      FinishReason reason, const RatingChange& rating_change) = 0;
    virtual void cleanup_finished_players(const FinishedPlayerState& white,
                                          const FinishedPlayerState& black) = 0;
};

}  // namespace kfc
