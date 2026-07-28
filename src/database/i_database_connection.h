#pragma once

struct sqlite3;

namespace kfc {

class IDatabaseConnection {
public:
    virtual ~IDatabaseConnection() = default;

    [[nodiscard]] virtual bool is_connected() const = 0;
    [[nodiscard]] virtual sqlite3* connection() = 0;
};

}  // namespace kfc
