#include "board.h"

#include <istream>
#include <sstream>
#include <ostream>
#include <string>

namespace kfc {

namespace {

std::string trim_line(const std::string& line) {
    const std::size_t start = line.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    const std::size_t end = line.find_last_not_of(" \t\r\n");
    return line.substr(start, end - start + 1);
}

Row split_row(const std::string& line) {
    Row tokens;
    std::istringstream stream(line);
    std::string token;
    while (stream >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

}  // namespace

bool is_valid_token(const std::string& token) noexcept {
    if (token == ".") {
        return true;
    }
    if (token.size() != 2) {
        return false;
    }

    const char color = token[0];
    const char piece = token[1];
    if (color != 'w' && color != 'b') {
        return false;
    }

    switch (piece) {
        case 'K':
        case 'Q':
        case 'R':
        case 'B':
        case 'N':
        case 'P':
            return true;
        default:
            return false;
    }
}

bool is_valid_board(const Board& board) noexcept {
    if (board.empty()) {
        return false;
    }

    const std::size_t width = board.front().size();
    if (width == 0) {
        return false;
    }

    for (const Row& row : board) {
        if (row.size() != width) {
            return false;
        }
        for (const std::string& token : row) {
            if (!is_valid_token(token)) {
                return false;
            }
        }
    }

    return true;
}

BoardError parse_board_rows(const std::vector<std::string>& lines, Board& board) {
    board.clear();

    if (lines.empty()) {
        return BoardError::Ok;
    }

    std::size_t expected_width = 0;
    for (const std::string& line : lines) {
        const Row row = split_row(line);
        if (expected_width == 0) {
            expected_width = row.size();
            if (expected_width == 0) {
                return BoardError::RowWidthMismatch;
            }
        } else if (row.size() != expected_width) {
            return BoardError::RowWidthMismatch;
        }

        for (const std::string& token : row) {
            if (!is_valid_token(token)) {
                return BoardError::UnknownToken;
            }
        }

        board.push_back(row);
    }

    return BoardError::Ok;
}

const char* board_error_message(BoardError error) noexcept {
    switch (error) {
        case BoardError::UnknownToken:
            return "ERROR UNKNOWN_TOKEN";
        case BoardError::RowWidthMismatch:
            return "ERROR ROW_WIDTH_MISMATCH";
        case BoardError::Ok:
        default:
            return "";
    }
}

VplInput read_vpl_input(std::istream& in) {
    VplInput result;

    enum class Section { None, Board, Commands } section = Section::None;
    std::vector<std::string> board_lines;

    std::string line;
    while (std::getline(in, line)) {
        const std::string trimmed = trim_line(line);
        if (trimmed == "Board:") {
            section = Section::Board;
            continue;
        }
        if (trimmed == "Commands:") {
            section = Section::Commands;
            continue;
        }

        if (section == Section::Board) {
            board_lines.push_back(line);
        } else if (section == Section::Commands) {
            if (!line.empty()) {
                result.commands.push_back(line);
            }
        }
    }

    result.error = parse_board_rows(board_lines, result.board);
    return result;
}

void write_board(std::ostream& out, const Board& board) {
    for (std::size_t row = 0; row < board.size(); ++row) {
        for (std::size_t col = 0; col < board[row].size(); ++col) {
            if (col > 0) {
                out << ' ';
            }
            out << board[row][col];
        }
        if (row + 1 < board.size()) {
            out << '\n';
        }
    }
}

}  // namespace kfc
