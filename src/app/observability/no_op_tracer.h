#pragma once

#include "app/observability/i_tracer.h"

namespace kfc::app::observability {

class NoOpTracer final : public ITracer {
public:
    void start_span(std::string_view) override {}
    void end_span() override {}
    void add_span_attribute(std::string_view, std::string_view) override {}
};

}  // namespace kfc::app::observability
