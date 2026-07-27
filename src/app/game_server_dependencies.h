#pragma once

namespace kfc {

class AuthenticationService;
class IGameRepository;
class IUserRepository;
class SqliteDatabase;

namespace app {

struct GameServerDependencies {
    SqliteDatabase& database;
    IUserRepository& user_repository;
    IGameRepository& game_repository;
    AuthenticationService& authentication_service;
};

}  // namespace app
}  // namespace kfc
