#pragma once

#include <map>
#include <string>
#include <string_view>

namespace kfc::app::observability {

enum class LogLevel {
    Info,
    Warn,
    Error,
};

class StructuredLogger {
public:
    void configure(std::string service, std::string server_id);

    void log(LogLevel level, std::string_view event,
             const std::map<std::string, std::string>& fields = {}) const;

private:
    std::string service_;
    std::string server_id_;
};

[[nodiscard]] StructuredLogger& logger();

}  // namespace kfc::app::observability
