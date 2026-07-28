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

On first startup against an empty database, the server automatically creates the `players` and `games` tables through `PostgresConnection::initialize_schema()`. No manual SQL is required.
