#include "model/board_model.h"
#include "rules/board_validator.h"
#include "io/board_writer.h"
#include "model/game_config.h"
#include "io/vpl_io.h"
#include "test_helpers.h"

#include <doctest/doctest.h>
#include <sstream>
#include <string>
#include <vector>

TEST_CASE("VplIoTest - ReadAndWriteRoundtrip") {
    const std::string input =
        "Board:\n"
        "wK . bQ\n"
        ". wN .\n"
        "bP . wR\n"
        "Commands:\n"
        "print board\n";

    std::istringstream in(input);
    const kfc::VplInput parsed = kfc::read_vpl_input(in);
    CHECK_EQ(parsed.error, kfc::BoardError::Ok);
    CHECK(parsed.board.is_valid());
    CHECK_EQ(parsed.commands.size(), 1);
    CHECK_EQ(parsed.commands[0], "print board");

    std::ostringstream output;
    kfc::write_board(output, parsed.board);
    CHECK_EQ(output.str(), "wK . bQ\n. wN .\nbP . wR");
}

TEST_CASE("VplIoTest - InvalidBoardProducesError") {
    const std::string input =
        "Board:\n"
        "wK xZ\n"
        ". .\n"
        "Commands:\n";

    std::istringstream in(input);
    const kfc::VplInput parsed = kfc::read_vpl_input(in);
    CHECK_EQ(parsed.error, kfc::BoardError::UnknownToken);
    CHECK_EQ(std::string(kfc::board_error_message(parsed.error)), kfc::kErrorUnknownToken);
}

TEST_CASE("VplIoTest - ReadVplMultipleCommandsAndSkipBlankLines") {
    const std::string input =
        "Board:\n"
        "wK . bK\n"
        "Commands:\n"
        "wait 100\n"
        "\n"
        "print board\n";

    std::istringstream in(input);
    const kfc::VplInput parsed = kfc::read_vpl_input(in);
    CHECK_EQ(parsed.error, kfc::BoardError::Ok);
    CHECK_EQ(parsed.commands.size(), 2);
    CHECK_EQ(parsed.commands[0], "wait 100");
    CHECK_EQ(parsed.commands[1], "print board");
}

TEST_CASE("VplIoTest - ReadVplNoSectionHeaders") {
    const std::string input = "wK . bK\n";
    std::istringstream in(input);
    const kfc::VplInput parsed = kfc::read_vpl_input(in);
    CHECK_EQ(parsed.error, kfc::BoardError::Ok);
    CHECK_EQ(parsed.board.rows(), 0);
    CHECK(parsed.commands.empty());
}

TEST_CASE("VplIoTest - ReadVplWhitespacePaddedBoardLine") {
    const std::string input =
        "Board:\n"
        "  wK . bK  \n"
        "Commands:\n";

    std::istringstream in(input);
    const kfc::VplInput parsed = kfc::read_vpl_input(in);
    CHECK_EQ(parsed.error, kfc::BoardError::Ok);
    CHECK_EQ(parsed.board.token_at(0, 0), "wK");
    CHECK_EQ(parsed.board.token_at(0, 2), "bK");
}
