#include "network/snapshot_reader.h"

#include "model/piece_token.h"

#include <cctype>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

namespace kfc {

namespace {

std::vector<std::string> split_lines(std::string_view text) {
    std::vector<std::string> lines;
    std::string current;

    for (const char ch : text) {
        if (ch == '\r') {
            continue;
        }
        if (ch == '\n') {
            lines.push_back(std::move(current));
            current.clear();
            continue;
        }
        current.push_back(ch);
    }

    if (!current.empty()) {
        lines.push_back(std::move(current));
    }

    return lines;
}

std::vector<std::string> split_tokens(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream stream(line);
    std::string token;
    while (stream >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

std::optional<std::int64_t> parse_int64(std::string_view token) {
    if (token.empty()) {
        return std::nullopt;
    }

    std::int64_t value = 0;
    for (const char ch : token) {
        if (!std::isdigit(static_cast<unsigned char>(ch))) {
            return std::nullopt;
        }
        value = value * 10 + static_cast<std::int64_t>(ch - '0');
    }
    return value;
}

std::optional<std::size_t> parse_non_negative_size(std::string_view token) {
    const auto parsed = parse_int64(token);
    if (!parsed || *parsed < 0) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(*parsed);
}

std::optional<bool> parse_bool(std::string_view token) {
    if (token == "true") {
        return true;
    }
    if (token == "false") {
        return false;
    }
    return std::nullopt;
}

std::optional<CellView> parse_cell_token(const std::string& token) {
    if (!is_valid_token(token)) {
        return std::nullopt;
    }

    CellView cell;
    if (const std::optional<PieceDescriptor> descriptor = descriptor_from_token(token)) {
        cell.piece = PieceView{descriptor->kind, descriptor->color};
    }
    return cell;
}

}  // namespace

std::optional<BoardViewModel> read_snapshot(std::string_view text) {
    const std::vector<std::string> lines = split_lines(text);
    if (lines.empty() || lines.front() != "snapshot") {
        return std::nullopt;
    }

    BoardViewModel view;
    bool saw_cells_marker = false;
    std::size_t line_index = 1;

    for (; line_index < lines.size(); ++line_index) {
        const std::vector<std::string> tokens = split_tokens(lines[line_index]);
        if (tokens.empty()) {
            continue;
        }

        if (tokens[0] == "cells") {
            if (tokens.size() != 1) {
                return std::nullopt;
            }
            saw_cells_marker = true;
            ++line_index;
            break;
        }

        if (tokens[0] == "clock_ms") {
            if (tokens.size() != 2) {
                return std::nullopt;
            }
            const auto parsed = parse_int64(tokens[1]);
            if (!parsed) {
                return std::nullopt;
            }
            view.clock_ms = *parsed;
            continue;
        }

        if (tokens[0] == "game_over") {
            if (tokens.size() != 2) {
                return std::nullopt;
            }
            const auto parsed = parse_bool(tokens[1]);
            if (!parsed) {
                return std::nullopt;
            }
            view.game_over = *parsed;
            continue;
        }

        if (tokens[0] == "height") {
            if (tokens.size() != 2) {
                return std::nullopt;
            }
            const auto parsed = parse_non_negative_size(tokens[1]);
            if (!parsed) {
                return std::nullopt;
            }
            view.height = *parsed;
            continue;
        }

        if (tokens[0] == "width") {
            if (tokens.size() != 2) {
                return std::nullopt;
            }
            const auto parsed = parse_non_negative_size(tokens[1]);
            if (!parsed) {
                return std::nullopt;
            }
            view.width = *parsed;
            continue;
        }

        if (tokens[0] == "selection") {
            if (tokens.size() != 3) {
                return std::nullopt;
            }
            const auto row = parse_non_negative_size(tokens[1]);
            const auto col = parse_non_negative_size(tokens[2]);
            if (!row || !col) {
                return std::nullopt;
            }
            view.selection = std::make_pair(*row, *col);
            continue;
        }

        return std::nullopt;
    }

    if (!saw_cells_marker) {
        return std::nullopt;
    }

    view.cells.clear();
    view.cells.reserve(view.height * view.width);

    for (std::size_t row = 0; row < view.height; ++row) {
        if (line_index >= lines.size()) {
            return std::nullopt;
        }

        const std::vector<std::string> tokens = split_tokens(lines[line_index++]);
        if (tokens.size() != view.width) {
            return std::nullopt;
        }

        for (const std::string& token : tokens) {
            const std::optional<CellView> cell = parse_cell_token(token);
            if (!cell) {
                return std::nullopt;
            }
            view.cells.push_back(*cell);
        }
    }

    if (line_index != lines.size()) {
        return std::nullopt;
    }

    return view;
}

}  // namespace kfc
