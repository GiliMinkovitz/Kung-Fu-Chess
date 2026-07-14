#include "model/position.h"

#include <doctest/doctest.h>
#include <sstream>
#include <string>

TEST_CASE("PositionTest - PositionEqualitySameCoordinates") {
    const kfc::Position left{2, 3};
    const kfc::Position right{2, 3};
    CHECK_EQ(left, right);
}

TEST_CASE("PositionTest - PositionEqualityDifferentCoordinates") {
    const kfc::Position base{2, 3};
    const kfc::Position different_row{3, 3};
    const kfc::Position different_col{2, 4};
    CHECK_NE(base, different_row);
    CHECK_NE(base, different_col);
}

TEST_CASE("PositionTest - PositionReadableRepresentation") {
    const kfc::Position pos{4, 5};
    std::ostringstream out;
    out << pos;
    CHECK_EQ(out.str(), "(4, 5)");
}
