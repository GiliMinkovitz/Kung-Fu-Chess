#include "vpl_io.h"

#include "piece.h"

#include <istream>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

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

void write_board(std::ostream& out, const BoardModel& board) {
    for (std::size_t row = 0; row < board.rows(); ++row) {
        for (std::size_t col = 0; col < board.cols(); ++col) {
            if (col > 0) {
                out << ' ';
            }
            out << board.token_at(row, col);
        }
        if (row + 1 < board.rows()) {
            out << '\n';
        }
    }
}

}  // namespace kfc
