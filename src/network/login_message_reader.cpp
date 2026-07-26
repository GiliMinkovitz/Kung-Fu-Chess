#include "network/login_message_reader.h"

#include <sstream>
#include <string>
#include <vector>

namespace kfc {

namespace {

std::string first_line(std::string_view text) {
    std::string line;
    for (const char ch : text) {
        if (ch == '\r') {
            continue;
        }
        if (ch == '\n') {
            break;
        }
        line.push_back(ch);
    }
    return line;
}

std::vector<std::string> split_tokens(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream stream{line};
    std::string token;
    while (stream >> token) {
        tokens.push_back(std::move(token));
    }
    return tokens;
}

}  // namespace

std::optional<LoginResult> read_login_message(std::string_view text) {
    const std::vector<std::string> tokens = split_tokens(first_line(text));
    if (tokens.empty()) {
        return std::nullopt;
    }

    if (tokens[0] == "login_ok" && tokens.size() >= 2) {
        try {
            const int rating = std::stoi(tokens[1]);
            return LoginResult{LoginResultStatus::Ok, rating, ""};
        } catch (const std::exception&) {
            return std::nullopt;
        }
    }

    if (tokens[0] == "login_failed" && tokens.size() >= 2) {
        std::string reason = tokens[1];
        for (std::size_t i = 2; i < tokens.size(); ++i) {
            reason += ' ';
            reason += tokens[i];
        }
        return LoginResult{LoginResultStatus::Failed, 0, reason};
    }

    return std::nullopt;
}

}  // namespace kfc
