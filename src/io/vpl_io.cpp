#include "vpl_io.h"

#include "../model/game_config.h"

#include <istream>
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

}  // namespace

VplInput read_vpl_input(std::istream& in) {
    VplInput result;

    enum class Section { None, Board, Commands } section = Section::None;
    std::vector<std::string> board_lines;

    std::string line;
    while (std::getline(in, line)) {
        const std::string trimmed = trim_line(line);
        if (trimmed == kBoardSectionHeader) {
            section = Section::Board;
            continue;
        }
        if (trimmed == kCommandsSectionHeader) {
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

}  // namespace kfc
