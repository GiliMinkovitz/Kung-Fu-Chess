#pragma once

#include "database/game_repository.h"

namespace kfc {

class SqliteGameRepository final : public GameRepository {
public:
    using GameRepository::GameRepository;
};

}  // namespace kfc
