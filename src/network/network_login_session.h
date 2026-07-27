#pragma once

#include "network/network_input_handler.h"

#include <optional>
#include <string>
#include <string_view>

namespace kfc {

enum class NetworkLoginPhase {
    Connected,
    LoginSent,
    LoginOk,
    PlayRequested,
    LoginFailed,
};

// Manages the client login handshake: send login, wait for login_ok, then play.
class NetworkLoginSession {
public:
    explicit NetworkLoginSession(NetworkInputHandler& handler);

    [[nodiscard]] static std::string resolve_username(const std::string& username_input);

    bool send_login(const std::string& username_input);
    void handle_message(std::string_view message);
    bool try_send_play();
    void reset() noexcept;

    [[nodiscard]] NetworkLoginPhase phase() const noexcept { return phase_; }
    [[nodiscard]] bool is_play_requested() const noexcept {
        return phase_ == NetworkLoginPhase::PlayRequested;
    }
    [[nodiscard]] bool is_login_failed() const noexcept {
        return phase_ == NetworkLoginPhase::LoginFailed;
    }
    [[nodiscard]] const std::string& login_failure_reason() const noexcept {
        return failure_reason_;
    }
    [[nodiscard]] int rating() const noexcept { return rating_; }

private:
    NetworkInputHandler& handler_;
    NetworkLoginPhase phase_ = NetworkLoginPhase::Connected;
    std::string failure_reason_;
    int rating_ = 0;
};

}  // namespace kfc
