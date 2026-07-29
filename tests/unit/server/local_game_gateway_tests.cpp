#include "model/piece.h"
#include "server/gateway/game_redirect_info.h"
#include "server/gateway/local_game_gateway.h"
#include "server/network/i_message_sink.h"
#include "server/network/player_id.h"

#include <doctest/doctest.h>

#include <string>
#include <utility>
#include <vector>

namespace {

class RecordingMessageSink : public kfc::IMessageSink {
public:
    bool send(kfc::PlayerId player, std::string_view message) override {
        sent_messages_.push_back({player, std::string(message)});
        return true;
    }

    bool send_message(kfc::PlayerId player, std::string_view message) override {
        return send(player, message);
    }

    struct SentMessage {
        kfc::PlayerId player;
        std::string message;
    };

    [[nodiscard]] const std::vector<SentMessage>& sent_messages() const noexcept {
        return sent_messages_;
    }

private:
    std::vector<SentMessage> sent_messages_;
};

}  // namespace

TEST_CASE("LocalGameGatewayTest - FormatsGameRedirectForWhite") {
    RecordingMessageSink sink;
    kfc::LocalGameGateway gateway{sink};

    gateway.send_game_redirect(
        1, kfc::GameRedirectInfo{42, "server-a", "ws://localhost:8765", kfc::PieceColor::White});

    REQUIRE_EQ(sink.sent_messages().size(), 1u);
    CHECK_EQ(sink.sent_messages()[0].player, 1u);
    CHECK_EQ(sink.sent_messages()[0].message, "game_redirect ws://localhost:8765 42 white");
}

TEST_CASE("LocalGameGatewayTest - FormatsGameRedirectForBlack") {
    RecordingMessageSink sink;
    kfc::LocalGameGateway gateway{sink};

    gateway.send_game_redirect(
        2, kfc::GameRedirectInfo{7, "server-b", "ws://games.example:9000", kfc::PieceColor::Black});

    REQUIRE_EQ(sink.sent_messages().size(), 1u);
    CHECK_EQ(sink.sent_messages()[0].player, 2u);
    CHECK_EQ(sink.sent_messages()[0].message, "game_redirect ws://games.example:9000 7 black");
}
