# Kung Fu Chess — Docker (Phase 14.4)

Independent **Gateway**, **Matchmaker**, and **Game Server** containers with **Redis** as the shared runtime registry.

```
Client
  |
  v
Gateway container  (KungFuChessGateway)
  |
  |  HTTP POST /matchmaking/join
  v
Matchmaker container  (KungFuChessMatchmaker)
  |
  +--> Redis  (queue + routing registry)
  |
  +--> Game Server  HTTP POST /allocate
```

## Prerequisites

- Docker Engine with Compose v2
- Ports available on the host: `8765`, `8766`, `8080`, `8081`, `6379`

Matchmaker and gateway internal ports (`8770`, `8771`, `8772`) are **not** published to the host.

## Build images

From the repository root:

```bash
docker compose build
```

Build individual services:

```bash
docker compose build gateway
docker compose build matchmaker
docker compose build game-server
```

## Run the local cloud stack

```bash
docker compose up
```

Run detached:

```bash
docker compose up -d
```

Stop and remove containers:

```bash
docker compose down
```

## Architecture

| Service | Image | Role |
|---------|-------|------|
| `gateway` | `docker/Dockerfile.gateway` | Lobby WebSocket, auth, forwards matchmaking via HTTP |
| `matchmaker` | `docker/Dockerfile.matchmaker` | Queue, pairing, game-server selection, allocation |
| `game-server` | `docker/Dockerfile.game_server` | Room simulation, allocation API, game WebSocket |
| `redis` | `redis:7-alpine` | Queue data, routing registry, game-server heartbeats |

Gateway and Matchmaker **do not share** a SQLite file with the game server. Each has its own volume:

- `gateway_data` → `/data/kfc.db` (auth)
- `matchmaker_data` → `/data/kfc.db` (game record creation for matches)
- `game_server_data` → `/data/kfc.db` (game results / ratings)

## Required environment variables

### Gateway

| Variable | Compose default | Purpose |
|----------|-------------------|---------|
| `KFC_BIND_ADDRESS` | `0.0.0.0` | Listen address inside container |
| `KFC_PORT` | `8765` | Lobby WebSocket port |
| `KFC_HEALTH_PORT` | `8080` | Health HTTP port |
| `KFC_GATEWAY_INTERNAL_PORT` | `8771` | Internal notification HTTP port (matchmaker → gateway) |
| `KFC_GATEWAY_SERVER_ID` | `gateway-1` | Gateway identity |
| `KFC_MATCHMAKER_ENDPOINT` | `http://matchmaker:8770` | Matchmaker HTTP API (Docker DNS) |
| `KFC_DB_PATH` | `/data/kfc.db` | SQLite path (gateway auth) |
| `KFC_REDIS_ENABLED` | `true` | Enable Redis runtime store |
| `KFC_REDIS_HOST` | `redis` | Redis service name |

### Matchmaker

| Variable | Compose default | Purpose |
|----------|-------------------|---------|
| `KFC_BIND_ADDRESS` | `0.0.0.0` | Listen address inside container |
| `KFC_SERVER_ID` | `matchmaker-1` | Matchmaker identity |
| `KFC_MATCHMAKER_PORT` | `8770` | Matchmaking HTTP API |
| `KFC_MATCHMAKER_HEALTH_PORT` | `8772` | Health HTTP port |
| `KFC_GATEWAY_NOTIFICATION_ENDPOINT` | `http://gateway:8771` | Gateway notification API |
| `KFC_DB_PATH` | `/data/kfc.db` | SQLite path (game records) |
| `KFC_REDIS_ENABLED` | `true` | Queue + registry |
| `KFC_REDIS_HOST` | `redis` | Redis service name |
| `KFC_INTERNAL_SERVICE_TOKEN` | *(shared secret)* | Allocation API auth |

### Game Server

| Variable | Compose default | Purpose |
|----------|-------------------|---------|
| `KFC_BIND_ADDRESS` | `0.0.0.0` | Listen address inside container |
| `KFC_SERVER_ID` | `game-server-1` | Game server identity |
| `KFC_GAME_PORT` | `8766` | Game WebSocket port |
| `KFC_GAME_INTERNAL_PORT` | `8767` | Allocation HTTP port |
| `KFC_GAME_HEALTH_PORT` | `8081` | Health HTTP port |
| `KFC_GAME_ENDPOINT` | `ws://localhost:8766` | Client-facing WebSocket URL (host-mapped) |
| `KFC_ALLOCATION_ENDPOINT` | `http://game-server:8767/allocate` | Internal allocation URL |
| `KFC_DB_PATH` | `/data/kfc.db` | SQLite path (game DB) |
| `KFC_REDIS_ENABLED` | `true` | Publish heartbeat to Redis |
| `KFC_REDIS_HOST` | `redis` | Redis service name |
| `KFC_INTERNAL_SERVICE_TOKEN` | *(shared secret)* | Allocation API auth |

### Shared / optional

| Variable | Default | Purpose |
|----------|---------|---------|
| `KFC_REDIS_PORT` | `6379` | Redis port |
| `KFC_ALLOCATION_TIMEOUT_MS` | `2000` | Allocation HTTP timeout |
| `KFC_ALLOCATION_RETRY_COUNT` | `3` | Allocation retry count |
| `KFC_GAME_SERVER_TTL_SECONDS` | `10` | Heartbeat TTL for registry |
| `KFC_REGION` | `local` | Region label |
| `KFC_DIAGNOSTICS` | `true` | Verbose logging |

## Health endpoints

### Gateway (`8080`)

```bash
curl http://localhost:8080/health
curl http://localhost:8080/ready
curl http://localhost:8080/metrics
```

### Matchmaker (`8772`, internal only from host unless port-mapped)

```bash
docker compose exec matchmaker curl -fsS http://127.0.0.1:8772/health
docker compose exec matchmaker curl -fsS http://127.0.0.1:8772/ready
docker compose exec matchmaker curl -fsS http://127.0.0.1:8772/metrics
```

Matchmaker `/ready` requires Redis **and** at least one registered game server.

### Game Server (`8081`)

```bash
curl http://localhost:8081/health
curl http://localhost:8081/ready
curl http://localhost:8081/metrics
```

## Connect a client

1. WebSocket to Gateway: `ws://localhost:8765`
2. Login / search for match through the gateway
3. Gateway forwards join to Matchmaker; on match, Matchmaker notifies Gateway, which sends `game_redirect ws://localhost:8766 <room_id> <side>`
4. Connect to the game server WebSocket and send `join_game <room_id>`

## Notes

- `KFC_GAME_ENDPOINT` uses `localhost` because clients connect from the **host**, not from inside Docker network.
- Service-to-service URLs use Docker DNS names (`matchmaker`, `gateway`, `game-server`).
- Do not set `KFC_BIND_ADDRESS` to `127.0.0.1` inside containers; use `0.0.0.0`.
- The legacy root `Dockerfile` builds the monolithic `KungFuChessServer` and is not used by this compose stack.
