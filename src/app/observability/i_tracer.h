#pragma once

#include <string>
#include <string_view>

namespace kfc::app::observability {

class ITracer {
public:
    virtual ~ITracer() = default;

    virtual void start_span(std::string_view operation) = 0;
    virtual void end_span() = 0;
    virtual void add_span_attribute(std::string_view key, std::string_view value) = 0;
};

}  // namespace kfc::app::observability
