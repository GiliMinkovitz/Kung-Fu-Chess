#pragma once

#include <string>

namespace kfc {

class PasswordHasher {
public:
    static constexpr int kDefaultIterations = 10000;

    [[nodiscard]] static std::string hash_password(const std::string& password,
                                                   int iterations = kDefaultIterations);
    [[nodiscard]] static bool verify_password(const std::string& password,
                                              const std::string& stored_hash);
};

}  // namespace kfc
