#pragma once

#include "database/sqlite_database.h"
#include "server/player.h"

#include <optional>
#include <string>

namespace kfc {

class PlayerRepository {
public:
    explicit PlayerRepository(SqliteDatabase& database);

    [[nodiscard]] std::optional<Player> find_by_username(const std::string& username);
    [[nodiscard]] std::optional<Player> create_player(const std::string& username,
                                                      int rating = 1000);
    [[nodiscard]] bool update_rating(int player_id, int rating);

private:
    SqliteDatabase& database_;
};

}  // namespace kfc
