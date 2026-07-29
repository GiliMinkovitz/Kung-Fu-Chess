#pragma once

#include "app/observability/correlation_id.h"
#include "app/observability/i_tracer.h"
#include "app/observability/metric_counters.h"
#include "app/observability/no_op_tracer.h"
#include "app/observability/structured_logger.h"

#include <boost/beast/http/message.hpp>

#include <string>
#include <string_view>

namespace kfc::app::observability {

template<class Body, class Fields>
void extract_correlation_id_from_request(
    const boost::beast::http::request<Body, Fields>& request) {
    const auto it = request.find(kCorrelationIdHeader);
    if (it != request.end() && !it->value().empty()) {
        set_correlation_id(std::string(it->value()));
        return;
    }

    if (current_correlation_id().empty()) {
        set_correlation_id(generate_correlation_id());
    }
}

void configure_observability(std::string service, std::string server_id);

[[nodiscard]] ITracer& tracer();

}  // namespace kfc::app::observability
