#include "board_model.h"
#include "board_writer.h"
#include "test_helpers.h"

#include <doctest/doctest.h>
#include <sstream>

TEST_CASE("BoardWriterTest - WriteEmptyBoard") {
    std::ostringstream output;
    kfc::write_board(output, kfc::BoardModel{});
    CHECK(output.str().empty());
}

TEST_CASE("BoardWriterTest - WriteSingleRowBoard") {
    std::ostringstream output;
    kfc::write_board(output, kfc::test::make_board({{"wK", "bK"}}));
    CHECK_EQ(output.str(), "wK bK");
}
