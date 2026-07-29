#pragma once

#include <string>

namespace kfc::app::observability {

constexpr const char* kCorrelationIdHeader = "X-KFC-Correlation-Id";

[[nodiscard]] std::string generate_correlation_id();
[[nodiscard]] const std::string& current_correlation_id();
void set_correlation_id(std::string correlation_id);
void clear_correlation_id();

class CorrelationScope {
public:
    explicit CorrelationScope(std::string correlation_id);
    ~CorrelationScope();

    CorrelationScope(const CorrelationScope&) = delete;
    CorrelationScope& operator=(const CorrelationScope&) = delete;

private:
    std::string previous_;
    bool had_previous_ = false;
};

}  // namespace kfc::app::observability
