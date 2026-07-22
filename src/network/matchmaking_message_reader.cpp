#include "network/matchmaking_message_reader.h"

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

std::optional<MatchmakingState> read_matchmaking_message(std::string_view text) {
    const std::vector<std::string> tokens = split_tokens(first_line(text));
    if (tokens.empty()) {
        return std::nullopt;
    }

    if (tokens[0] == "searching") {
        return MatchmakingState::Searching;
    }
    if (tokens[0] == "search_timeout") {
        return MatchmakingState::Timeout;
    }
    if (tokens[0] == "match_found" && tokens.size() >= 2) {
        if (tokens[1] == "white") {
            return MatchmakingState::MatchedWhite;
        }
        if (tokens[1] == "black") {
            return MatchmakingState::MatchedBlack;
        }
    }

    return std::nullopt;
}

}  // namespace kfc
