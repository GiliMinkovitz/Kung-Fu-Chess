#include "path_utils.h"

#include <doctest/doctest.h>
#include <vector>

TEST_CASE("PathUtilsTest - ForEachCellOnPath") {
    std::vector<std::pair<int, int>> cells;
    kfc::for_each_cell_on_path(0, 0, 0, 3, [&](int row, int col) {
        cells.emplace_back(row, col);
    });
    CHECK_EQ(cells.size(), 2);
    CHECK_EQ(cells[0], std::make_pair(0, 1));
    CHECK_EQ(cells[1], std::make_pair(0, 2));

    cells.clear();
    kfc::for_each_cell_on_path(0, 0, 2, 2, [&](int row, int col) {
        cells.emplace_back(row, col);
    });
    CHECK_EQ(cells.size(), 1);
    CHECK_EQ(cells[0], std::make_pair(1, 1));
}

TEST_CASE("PathUtilsTest - PathsShareCellOnOverlappingRoutes") {
    CHECK(kfc::paths_share_cell({0, 0}, {0, 3}, {0, 1}, {0, 4}));
    CHECK(kfc::paths_share_cell({0, 0}, {2, 2}, {0, 2}, {2, 0}));
    CHECK_FALSE(kfc::paths_share_cell({0, 0}, {0, 2}, {1, 0}, {1, 2}));
}

TEST_CASE("PathUtilsTest - PathsShareCellNonStraightPaths") {
    CHECK(kfc::paths_share_cell({0, 0}, {0, 2}, {0, 2}, {2, 2}));
    CHECK_FALSE(kfc::paths_share_cell({0, 0}, {0, 1}, {2, 0}, {2, 1}));
}

TEST_CASE("PathUtilsTest - ForEachCellOnPathAdjacent") {
    std::vector<std::pair<int, int>> cells;
    kfc::for_each_cell_on_path(0, 0, 0, 1, [&](int row, int col) {
        cells.emplace_back(row, col);
    });
    CHECK(cells.empty());
}

TEST_CASE("PathUtilsTest - ForEachCellOnPathVertical") {
    std::vector<std::pair<int, int>> cells;
    kfc::for_each_cell_on_path(0, 0, 3, 0, [&](int row, int col) {
        cells.emplace_back(row, col);
    });
    CHECK_EQ(cells.size(), 2);
    CHECK_EQ(cells[0], std::make_pair(1, 0));
    CHECK_EQ(cells[1], std::make_pair(2, 0));
}

TEST_CASE("PathUtilsTest - PathsShareCellEndpointOverlap") {
    CHECK(kfc::paths_share_cell({0, 0}, {0, 1}, {0, 1}, {0, 2}));
}
