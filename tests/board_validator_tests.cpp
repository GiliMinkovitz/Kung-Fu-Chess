#include "board_model.h"
#include "board_validator.h"
#include "game_config.h"
#include "test_helpers.h"

#include <doctest/doctest.h>
#include <string>
#include <vector>

TEST_CASE("BoardValidatorTest - InvalidUnknownToken") {
    CHECK_FALSE(kfc::is_valid_token("xZ"));
    CHECK(kfc::is_valid_token("."));
    CHECK(kfc::is_valid_token("wK"));
    CHECK(kfc::is_valid_token("bQ"));
}

TEST_CASE("BoardValidatorTest - ParseBoardRowsSuccess") {
    const std::vector<std::string> lines = {"wK . . bK", ". . . .", "wR . . bR"};
    kfc::BoardModel board;
    CHECK_EQ(kfc::parse_board_rows(lines, board), kfc::BoardError::Ok);
    CHECK_EQ(board.rows(), 3);
    CHECK_EQ(board.cols(), 4);
    CHECK_EQ(board.token_at(0, 0), "wK");
    CHECK_EQ(board.token_at(2, 3), "bR");
}

TEST_CASE("BoardValidatorTest - ParseBoardUnknownToken") {
    const std::vector<std::string> lines = {"wK xZ", ". ."};
    kfc::BoardModel board;
    CHECK_EQ(kfc::parse_board_rows(lines, board), kfc::BoardError::UnknownToken);
}

TEST_CASE("BoardValidatorTest - ParseBoardRowWidthMismatch") {
    const std::vector<std::string> lines = {"wK . .", ". bK"};
    kfc::BoardModel board;
    CHECK_EQ(kfc::parse_board_rows(lines, board), kfc::BoardError::RowWidthMismatch);
}

TEST_CASE("BoardValidatorTest - BoardErrorMessageRowWidthMismatch") {
    CHECK_EQ(std::string(kfc::board_error_message(kfc::BoardError::RowWidthMismatch)), kfc::kErrorRowWidthMismatch);
}

TEST_CASE("BoardValidatorTest - BoardErrorMessageOk") {
    CHECK_EQ(std::string(kfc::board_error_message(kfc::BoardError::Ok)), "");
}

TEST_CASE("BoardValidatorTest - ParseBoardEmptyLines") {
    kfc::BoardModel board;
    CHECK_EQ(kfc::parse_board_rows({}, board), kfc::BoardError::Ok);
    CHECK_EQ(board.rows(), 0);
}

TEST_CASE("BoardValidatorTest - IsValidTokenInvalidColor") {
    CHECK_FALSE(kfc::is_valid_token("xK"));
    CHECK_FALSE(kfc::is_valid_token("zP"));
}

TEST_CASE("BoardValidatorTest - ParseBoardEmptyRowTokens") {
    const std::vector<std::string> lines = {""};
    kfc::BoardModel board;
    CHECK_EQ(kfc::parse_board_rows(lines, board), kfc::BoardError::RowWidthMismatch);
}
