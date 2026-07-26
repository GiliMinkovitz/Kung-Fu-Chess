#include "network/game_result_message_reader.h"

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

std::optional<GameResult> read_game_result_message(std::string_view text) {
    const std::vector<std::string> tokens = split_tokens(first_line(text));
    if (tokens.size() != 4 || tokens[0] != "game_result") {
        return std::nullopt;
    }

    bool won = false;
    if (tokens[1] == "win") {
        won = true;
    } else if (tokens[1] != "loss") {
        return std::nullopt;
    }

    try {
        const int rating = std::stoi(tokens[3]);
        return GameResult{won, tokens[2], rating};
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

}  // namespace kfc
