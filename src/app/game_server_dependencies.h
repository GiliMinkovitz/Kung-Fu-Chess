#pragma once

namespace kfc {

class AuthenticationService;
class IDatabaseConnection;
class IGameRepository;
class IRuntimeStore;
class IUserRepository;

namespace app {

struct GameServerDependencies {
    IDatabaseConnection& database;
    IUserRepository& user_repository;
    IGameRepository& game_repository;
    AuthenticationService& authentication_service;
    IRuntimeStore& runtime_store;
};

}  // namespace app
}  // namespace kfc
