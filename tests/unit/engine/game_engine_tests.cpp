#include "engine/game_engine.h"
#include "test_helpers.h"

#include <doctest/doctest.h>
#include <sstream>
#include <string>
#include <vector>

TEST_CASE("GameEngineTest - ExecutePrintBoardAddsNewline") {
    kfc::GameEngine engine(kfc::test::make_board({{"wK", ".", "bK"}}));
    std::ostringstream out;

    engine.execute("print board", out);
    CHECK_EQ(out.str(), "wK . bK\n");
}

TEST_CASE("GameEngineTest - ExecuteNonPrintCommandDoesNotAddNewline") {
    kfc::GameEngine engine(kfc::test::make_board({{"wK", ".", "bK"}}));
    std::ostringstream out;

    engine.execute("click 50 50", out);
    CHECK(out.str().empty());
    CHECK(engine.state().has_selection());
}

TEST_CASE("GameEngineTest - ExecuteAllRunsCommandsInOrder") {
    kfc::GameEngine engine(kfc::test::make_board({{"wK", ".", "bK"}, {".", ".", "."}}));
    std::ostringstream out;

    engine.execute_all({"click 50 50", "click 150 150", "wait 1000", "print board"}, out);
    CHECK_EQ(out.str(), ". . bK\n. wK .\n");
}

TEST_CASE("GameEngineTest - ExecuteAllEmptyCommandList") {
    kfc::GameEngine engine(kfc::test::make_board({{"wK", ".", "bK"}}));
    std::ostringstream out;

    engine.execute_all({}, out);
    CHECK(out.str().empty());
}
