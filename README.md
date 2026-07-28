# Kung Fu Chess

## Docker

The Docker setup runs **KungFuChessServer** with **PostgreSQL**. No manual database installation is required.

When running outside Docker, the server defaults to **SQLite** (`kfc.db` in the working directory). Docker overrides that through environment variables in `docker-compose.yml`.

### Build

```bash
docker compose build
```

### Run

```bash
docker compose up
```

This starts PostgreSQL, waits until it is healthy, then starts the server on port **8765**.

### Stop

```bash
docker compose down
```

To remove the PostgreSQL data volume as well:

```bash
docker compose down -v
```

### PostgreSQL schema

> **TODO:** PostgreSQL schema initialization is not yet implemented in the application. `PostgresConnection::initialize_schema()` currently only verifies connectivity; it does not create tables. Expected tables are documented in `src/server/database/postgres_user_repository.h` and `src/database/postgres_game_repository.h`. Apply migrations through future repository-layer tooling before relying on user registration or game persistence in Docker.

Until schema migration is implemented, the server will start and connect to PostgreSQL, but database-backed features will fail at runtime.
