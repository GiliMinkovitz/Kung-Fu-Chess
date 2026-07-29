#include "app/observability/structured_logger.h"

#include "app/observability/correlation_id.h"

#include <chrono>
#include <ctime>
#include <iostream>
#include <iomanip>
#include <sstream>

namespace kfc::app::observability {

namespace {

StructuredLogger g_logger;

std::string escape_json(std::string_view value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (const char c : value) {
        switch (c) {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            default:
                escaped += c;
                break;
        }
    }
    return escaped;
}

std::string level_name(const LogLevel level) {
    switch (level) {
        case LogLevel::Info:
            return "info";
        case LogLevel::Warn:
            return "warn";
        case LogLevel::Error:
            return "error";
    }
    return "info";
}

std::string utc_timestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t seconds = std::chrono::system_clock::to_time_t(now);
    std::tm utc_time{};
#if defined(_WIN32)
    gmtime_s(&utc_time, &seconds);
#else
    gmtime_r(&seconds, &utc_time);
#endif
    std::ostringstream stream;
    stream << std::put_time(&utc_time, "%Y-%m-%dT%H:%M:%SZ");
    return stream.str();
}

}  // namespace

void StructuredLogger::configure(std::string service, std::string server_id) {
    service_ = std::move(service);
    server_id_ = std::move(server_id);
}

void StructuredLogger::log(const LogLevel level, const std::string_view event,
                           const std::map<std::string, std::string>& fields) const {
    std::ostringstream stream;
    stream << '{';
    stream << "\"timestamp\":\"" << utc_timestamp() << '"';
    stream << ",\"service\":\"" << escape_json(service_) << '"';
    stream << ",\"server_id\":\"" << escape_json(server_id_) << '"';
    stream << ",\"level\":\"" << level_name(level) << '"';
    stream << ",\"event\":\"" << escape_json(event) << '"';

    const std::string& correlation_id = current_correlation_id();
    if (!correlation_id.empty()) {
        stream << ",\"correlation_id\":\"" << escape_json(correlation_id) << '"';
    }

    for (const auto& [key, value] : fields) {
        stream << ",\"" << escape_json(key) << "\":\"" << escape_json(value) << '"';
    }

    stream << '}';
    std::cout << stream.str() << '\n';
}

StructuredLogger& logger() {
    return g_logger;
}

}  // namespace kfc::app::observability
