# Kung Fu Chess

## Docker

The Docker setup runs **KungFuChessServer** with **PostgreSQL**. No manual database installation is required.

When running outside Docker, the server defaults to **SQLite** (`kfc.db` in the working directory). Docker overrides that through environment variables in `docker-compose.yml`.

### Ports

| Port | Purpose |
|------|---------|
| **8765** | WebSocket game server (`KFC_PORT`) |
| **8080** | Health and metrics HTTP server (`KFC_HEALTH_PORT`) |

### Health endpoints

The server starts a health HTTP server automatically on startup.

| Endpoint | Description |
|----------|-------------|
| `GET /health` | Liveness probe. Returns `200 OK` with body `OK`. |
| `GET /ready` | Readiness probe. Returns `200 OK` with body `OK`. |
| `GET /metrics` | Plain-text server metrics (active rooms, connected sessions, matchmaking queue, uptime, last tick duration). |

Example:

```bash
curl http://localhost:8080/health
curl http://localhost:8080/ready
curl http://localhost:8080/metrics
```

Both the Docker image and the `server` Compose service define a health check that probes `GET /health` on port **8080**.

### Runtime environment variables

All variables are optional outside Docker; the server uses built-in defaults when a variable is unset.

| Variable | Default | Description |
|----------|---------|-------------|
| `KFC_PORT` | `8765` | WebSocket game server port |
| `KFC_BIND_ADDRESS` | `127.0.0.1` | Bind address for game and health servers (`0.0.0.0` in Docker) |
| `KFC_HEALTH_PORT` | `8080` | Health and metrics HTTP port |
| `KFC_MAX_CLIENTS` | `8` | Maximum concurrent client connections |
| `KFC_SERVER_ID` | `local` | Server identifier |
| `KFC_REGION` | `local` | Deployment region tag |
| `KFC_DIAGNOSTICS` | `true` | Enable diagnostic logging (`true`/`false`, `1`/`0`, `yes`/`no`, `on`/`off`) |
| `KFC_MATCH_MAX_RATING_DIFF` | `100` | Maximum rating difference for matchmaking |
| `KFC_MATCH_QUEUE_TIMEOUT_SEC` | `60` | Matchmaking queue timeout in seconds |
| `KFC_DB_BACKEND` | `sqlite` | Database backend: `sqlite` or `postgres` |
| `KFC_DB_PATH` | `kfc.db` | SQLite database file path (SQLite only) |
| `KFC_DB_HOST` | `localhost` | PostgreSQL host |
| `KFC_DB_PORT` | `5432` | PostgreSQL port |
| `KFC_DB_NAME` | `kfc` | PostgreSQL database name |
| `KFC_DB_USER` | `kfc` | PostgreSQL username |
| `KFC_DB_PASSWORD` | *(empty)* | PostgreSQL password |

### Build

```bash
docker compose build
```

### Run

```bash
docker compose up
```

This starts PostgreSQL, waits until it is healthy, then starts the server on port **8765** and the health server on port **8080**.

### Stop

```bash
docker compose down
```

To remove the PostgreSQL data volume as well:

```bash
docker compose down -v
```

### PostgreSQL schema

On first startup against an empty database, the server automatically creates the `players` and `games` tables through `PostgresConnection::initialize_schema()`. No manual SQL is required.
