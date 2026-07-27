#include "network/network_login_session.h"

#include "network/login_message_reader.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace kfc {

NetworkLoginSession::NetworkLoginSession(NetworkInputHandler& handler) : handler_{handler} {}

std::string NetworkLoginSession::resolve_username(const std::string& username_input) {
    if (!username_input.empty()) {
        return username_input;
    }

#ifdef _WIN32
    return "Player" + std::to_string(static_cast<unsigned long>(GetCurrentProcessId()));
#else
    return "Player" + std::to_string(static_cast<int>(getpid()));
#endif
}

bool NetworkLoginSession::send_login(const std::string& username_input) {
    if (phase_ != NetworkLoginPhase::Connected) {
        return false;
    }

    const std::string login_name = resolve_username(username_input);
    if (!handler_.send_login(login_name, login_name)) {
        return false;
    }

    phase_ = NetworkLoginPhase::LoginSent;
    return true;
}

void NetworkLoginSession::handle_message(std::string_view message) {
    if (phase_ != NetworkLoginPhase::LoginSent) {
        return;
    }

    const std::optional<LoginResult> result = read_login_message(message);
    if (!result.has_value()) {
        return;
    }

    if (result->status == LoginResultStatus::Ok) {
        rating_ = result->rating;
        phase_ = NetworkLoginPhase::LoginOk;
        return;
    }

    failure_reason_ = result->failure_reason;
    phase_ = NetworkLoginPhase::LoginFailed;
}

bool NetworkLoginSession::try_send_play() {
    if (phase_ != NetworkLoginPhase::LoginOk) {
        return false;
    }

    if (!handler_.send_play()) {
        return false;
    }

    phase_ = NetworkLoginPhase::PlayRequested;
    return true;
}

void NetworkLoginSession::reset() noexcept {
    phase_ = NetworkLoginPhase::Connected;
    failure_reason_.clear();
    rating_ = 0;
}

}  // namespace kfc
