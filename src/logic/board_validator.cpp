#include "logic/board_validator.h"

#include "core/game_config.h"
#include "core/piece.h"

#include <sstream>
#include <string>
#include <vector>

namespace kfc {

namespace {

std::vector<std::string> split_row(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream stream(line);
    std::string token;
    while (stream >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

}  // namespace

BoardError parse_board_rows(const std::vector<std::string>& lines, BoardModel& board) {
    BoardModel parsed;

    if (lines.empty()) {
        board = std::move(parsed);
        return BoardError::Ok;
    }

    std::size_t expected_width = 0;
    for (const std::string& line : lines) {
        const std::vector<std::string> tokens = split_row(line);
        if (expected_width == 0) {
            expected_width = tokens.size();
            if (expected_width == 0) {
                return BoardError::RowWidthMismatch;
            }
        } else if (tokens.size() != expected_width) {
            return BoardError::RowWidthMismatch;
        }

        std::vector<Piece> row;
        row.reserve(tokens.size());
        for (const std::string& token : tokens) {
            const std::optional<Piece> piece = Piece::from_token(token);
            if (!piece.has_value()) {
                return BoardError::UnknownToken;
            }
            row.push_back(*piece);
        }

        parsed.append_row(std::move(row));
    }

    board = std::move(parsed);
    return BoardError::Ok;
}

const char* board_error_message(BoardError error) noexcept {
    switch (error) {
        case BoardError::UnknownToken:
            return kErrorUnknownToken;
        case BoardError::RowWidthMismatch:
            return kErrorRowWidthMismatch;
        case BoardError::Ok:
        default:
            return "";
    }
}

}  // namespace kfc
