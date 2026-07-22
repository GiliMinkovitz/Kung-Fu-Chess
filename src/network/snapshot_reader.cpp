#include "network/snapshot_reader.h"

#include "model/piece_token.h"

#include <cctype>
#include <cstdlib>
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

std::optional<float> parse_float(std::string_view token) {
    if (token.empty()) {
        return std::nullopt;
    }

    const std::string buffer(token);
    char* end = nullptr;
    const float value = std::strtof(buffer.c_str(), &end);
    if (end == nullptr || end != buffer.c_str() + buffer.size()) {
        return std::nullopt;
    }
    return value;
}

std::optional<Piece::Id> parse_piece_id(std::string_view token) {
    const auto parsed = parse_int64(token);
    if (!parsed || *parsed < 0) {
        return std::nullopt;
    }
    return static_cast<Piece::Id>(*parsed);
}

std::optional<RestKind> parse_rest_kind(std::string_view token) {
    if (token == "short") {
        return RestKind::Short;
    }
    if (token == "long") {
        return RestKind::Long;
    }
    return std::nullopt;
}

bool is_section_marker(const std::vector<std::string>& tokens, std::string_view marker) {
    return tokens.size() == 1 && tokens[0] == marker;
}

std::optional<ActiveMoveSnapshot> parse_move_line(const std::vector<std::string>& tokens) {
    if (tokens.size() != 13 || tokens[0] != "move" || tokens[1] != "piece_id" ||
        tokens[3] != "piece" || tokens[5] != "from" || tokens[8] != "to" ||
        tokens[11] != "progress") {
        return std::nullopt;
    }

    const auto piece_id = parse_piece_id(tokens[2]);
    const auto descriptor = descriptor_from_token(tokens[4]);
    const auto from_row = parse_non_negative_size(tokens[6]);
    const auto from_col = parse_non_negative_size(tokens[7]);
    const auto to_row = parse_non_negative_size(tokens[9]);
    const auto to_col = parse_non_negative_size(tokens[10]);
    const auto progress = parse_float(tokens[12]);
    if (!piece_id || !descriptor || !from_row || !from_col || !to_row || !to_col ||
        !progress) {
        return std::nullopt;
    }

    return ActiveMoveSnapshot{*piece_id, descriptor->kind, descriptor->color, *from_row,
                              *from_col, *to_row,       *to_col,           *progress};
}

std::optional<ActiveJumpSnapshot> parse_jump_line(const std::vector<std::string>& tokens) {
    if (tokens.size() != 10 || tokens[0] != "jump" || tokens[1] != "piece_id" ||
        tokens[3] != "piece" || tokens[5] != "row" || tokens[8] != "progress") {
        return std::nullopt;
    }

    const auto piece_id = parse_piece_id(tokens[2]);
    const auto descriptor = descriptor_from_token(tokens[4]);
    const auto row = parse_non_negative_size(tokens[6]);
    const auto col = parse_non_negative_size(tokens[7]);
    const auto progress = parse_float(tokens[9]);
    if (!piece_id || !descriptor || !row || !col || !progress) {
        return std::nullopt;
    }

    return ActiveJumpSnapshot{*piece_id, descriptor->kind, descriptor->color, *row, *col,
                              *progress};
}

std::optional<ActiveRestSnapshot> parse_rest_line(const std::vector<std::string>& tokens) {
    if (tokens.size() != 10 || tokens[0] != "rest" || tokens[1] != "piece_id" ||
        tokens[3] != "row" || tokens[6] != "kind" || tokens[8] != "progress") {
        return std::nullopt;
    }

    const auto piece_id = parse_piece_id(tokens[2]);
    const auto row = parse_non_negative_size(tokens[4]);
    const auto col = parse_non_negative_size(tokens[5]);
    const auto kind = parse_rest_kind(tokens[7]);
    const auto progress = parse_float(tokens[9]);
    if (!piece_id || !row || !col || !kind || !progress) {
        return std::nullopt;
    }

    return ActiveRestSnapshot{*piece_id, *row, *col, *kind, *progress};
}

template <typename Snapshot, typename ParseLine>
bool parse_animation_section(std::string_view section_name, std::string_view next_section_name,
                             const std::vector<std::string>& lines, std::size_t& line_index,
                             ParseLine parse_line, std::vector<Snapshot>& destination) {
    if (line_index >= lines.size() ||
        !is_section_marker(split_tokens(lines[line_index]), section_name)) {
        return false;
    }
    ++line_index;

    while (line_index < lines.size()) {
        const std::vector<std::string> tokens = split_tokens(lines[line_index]);
        if (tokens.empty()) {
            ++line_index;
            continue;
        }
        if (!next_section_name.empty() && is_section_marker(tokens, next_section_name)) {
            return true;
        }

        const std::optional<Snapshot> parsed = parse_line(tokens);
        if (!parsed) {
            return false;
        }
        destination.push_back(*parsed);
        ++line_index;
    }

    return next_section_name.empty();
}

bool parse_animations(const std::vector<std::string>& lines, std::size_t& line_index,
                      AnimationSnapshot& animations) {
    if (line_index >= lines.size() ||
        !is_section_marker(split_tokens(lines[line_index]), "animations")) {
        return false;
    }
    ++line_index;

    if (!parse_animation_section("moves", "jumps", lines, line_index, parse_move_line,
                                 animations.moves)) {
        return false;
    }
    if (!parse_animation_section("jumps", "rests", lines, line_index, parse_jump_line,
                                 animations.jumps)) {
        return false;
    }
    if (!parse_animation_section("rests", "", lines, line_index, parse_rest_line,
                                 animations.rests)) {
        return false;
    }

    return line_index == lines.size();
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

    if (line_index < lines.size()) {
        if (!parse_animations(lines, line_index, view.animations)) {
            return std::nullopt;
        }
    }

    return view;
}

}  // namespace kfc
