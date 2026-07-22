#pragma once

#include <string>

namespace kfc {

class Player {
public:
    Player(int id, std::string username, int rating);

    [[nodiscard]] int id() const noexcept;
    [[nodiscard]] const std::string& username() const noexcept;
    [[nodiscard]] int rating() const noexcept;

private:
    int id_;
    std::string username_;
    int rating_;
};

}  // namespace kfc
