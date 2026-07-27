#pragma once

#include "database/sqlite_database.h"
#include "server/player.h"

#include <optional>
#include <string>

namespace kfc {

struct PlayerCredentials {
    int id;
    std::string username;
    int rating;
    std::string password_hash;
};

class PlayerRepository {
public:
    explicit PlayerRepository(SqliteDatabase& database);

    [[nodiscard]] std::optional<Player> find_by_username(const std::string& username) const;
    [[nodiscard]] std::optional<Player> find_by_id(int player_id) const;
    [[nodiscard]] std::optional<PlayerCredentials> find_credentials_by_username(
        const std::string& username) const;
    [[nodiscard]] std::optional<Player> create_player(const std::string& username,
                                                      int rating = 1000,
                                                      const std::string& password_hash = "");
    [[nodiscard]] bool update_rating(int player_id, int rating);

private:
    SqliteDatabase& database_;
};

}  // namespace kfc
