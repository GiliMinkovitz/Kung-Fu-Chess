#pragma once

namespace kfc {

class AuthenticationService;
class IDatabaseConnection;
class IGameRepository;
class IUserRepository;

namespace app {

struct GameServerDependencies {
    IDatabaseConnection& database;
    IUserRepository& user_repository;
    IGameRepository& game_repository;
    AuthenticationService& authentication_service;
};

}  // namespace app
}  // namespace kfc
