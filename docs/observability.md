# Kung Fu Chess — Observability (Phase 14.7)

Production observability foundation: Prometheus metrics, structured logging, request correlation IDs, and tracing hooks — without changing gameplay or protocols.

## Metrics

Every service exposes **`GET /metrics`** in [Prometheus text exposition format](https://prometheus.io/docs/instrumenting/exposition_formats/) on its health HTTP port.

| Service | Health port | Key metrics |
|---------|-------------|-------------|
| Gateway | 8080 | `connected_sessions`, `websocket_connections`, `authenticated_players`, `matchmaking_requests_total`, `matchmaking_failures_total` |
| Matchmaker | 8772 | `queue_size`, `matches_created_total`, `matchmaking_duration_seconds`, `allocation_failures_total`, `active_game_servers` |
| Game Server | 8081 | `active_rooms`, `active_players`, `snapshots_sent_total`, `player_actions_total`, `tick_duration_seconds`, `allocation_requests_total`, `allocation_api_active` |

Shared gauges on all services:

- `server_uptime_seconds`
- `redis_enabled`, `redis_connected`

### Example metrics (Gateway)

```text
# HELP connected_sessions Active gateway lobby sessions
# TYPE connected_sessions gauge
connected_sessions 3
# HELP authenticated_players Authenticated players in gateway lobby
# TYPE authenticated_players gauge
authenticated_players 2
# HELP matchmaking_requests_total Total matchmaking join requests forwarded by gateway
# TYPE matchmaking_requests_total counter
matchmaking_requests_total 14
```

### Scraping locally

```bash
curl http://localhost:8080/metrics   # Gateway
curl http://localhost:8772/metrics   # Matchmaker (port-forward in k8s)
curl http://localhost:8081/metrics   # Game Server
```

## Structured logging

Logs are emitted as **single-line JSON** to stdout with fixed fields:

| Field | Description |
|-------|-------------|
| `timestamp` | UTC ISO-8601 |
| `service` | `gateway`, `matchmaker`, `game-server`, or `monolith` |
| `server_id` | Instance identity from config |
| `level` | `info`, `warn`, or `error` |
| `event` | Event name (see below) |
| `correlation_id` | Present when a request flow is active |

### Event names

| Event | When |
|-------|------|
| `player_login` | Successful gateway authentication |
| `match_created` | Matchmaker paired players and allocated a room |
| `allocation_started` | Game allocation request begins |
| `allocation_failed` | Allocation failed (matchmaker or game server) |
| `game_finished` | Game server recorded a finished game |
| `room_destroyed` | Room cleaned up after game end |

Example log line:

```json
{"timestamp":"2026-07-29T12:34:56Z","service":"gateway","server_id":"gateway-1","level":"info","event":"player_login","correlation_id":"kfc-a1b2-c3d4","username":"alice","player_id":"42"}
```

## Correlation IDs

Cross-service requests propagate **`X-KFC-Correlation-Id`**.

Flow:

```
Gateway (generates ID on matchmaking join)
   → Matchmaker HTTP API
   → Game Server allocation API
```

The same ID appears in structured logs across all participating services. If an inbound HTTP request has no header, a new ID is generated at the service boundary.

## Health model

| Endpoint | Purpose |
|----------|---------|
| `GET /health` | Liveness — process is running |
| `GET /ready` | Readiness — dependencies available |

### Readiness checks

| Service | Checks |
|---------|--------|
| **Gateway** | PostgreSQL connected, Redis (if enabled), Matchmaker `/health` reachable |
| **Matchmaker** | PostgreSQL connected, Redis (if enabled), ≥1 registered game server |
| **Game Server** | PostgreSQL connected, Redis (if enabled), allocation API listener active |

```bash
curl http://localhost:8080/ready
curl http://localhost:8772/ready
curl http://localhost:8081/ready
```

Returns `200 OK` when ready, `503 Service Unavailable` otherwise.

## Tracing (future)

Tracing is prepared via `kfc::app::observability::ITracer` with a no-op default implementation. No Jaeger or OpenTelemetry integration yet — plug a real tracer in later without changing call sites.

```cpp
kfc::app::observability::tracer().start_span("allocate_game");
// ...
kfc::app::observability::tracer().end_span();
```

## Kubernetes

Pod templates for gateway, matchmaker, and game-server include Prometheus scrape annotations:

```yaml
prometheus.io/scrape: "true"
prometheus.io/port: "<health-port>"
prometheus.io/path: "/metrics"
```

Prometheus itself is **not** installed in this phase — annotations are ready for a future Prometheus Operator or scrape config.

## Implementation layout

```
src/app/observability/
├── correlation_id.h/cpp
├── structured_logger.h/cpp
├── metric_counters.h/cpp
├── prometheus_formatter.h/cpp
├── readiness_checker.h/cpp
├── i_tracer.h
├── no_op_tracer.h
└── observability.h/cpp
```

## Future phases

- **14.8** Horizontal Pod Autoscaler (uses these metrics)
- **14.9** Multi-game-server load balancing
- **14.10** Multi-region deployment
