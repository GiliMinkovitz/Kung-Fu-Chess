#pragma once

#include "server/matchmaking/match_created_handler.h"

#include <string>

namespace kfc {

class IGameHost;
class IGameRepository;
class IMessageSink;
class IRuntimeStore;
class MatchmakingService;
class PlayerSession;

class MatchLifecycleHandler : public IMatchCreatedHandler {
public:
    MatchLifecycleHandler(IGameHost& game_host, IGameRepository& game_repository,
                          IRuntimeStore& runtime_store, std::string server_id);

    void bind_matchmaking_service(MatchmakingService& matchmaking_service);
    void bind_message_sink(IMessageSink& message_sink);

    RoomId create_match(PlayerSession* white, PlayerSession* black) override;
    void notify_match_created(const MatchCreated& match);
    void process_timeouts();

private:
    IGameHost& game_host_;
    IGameRepository& game_repository_;
    IRuntimeStore& runtime_store_;
    std::string server_id_;
    MatchmakingService* matchmaking_service_ = nullptr;
    IMessageSink* message_sink_ = nullptr;
};

}  // namespace kfc
