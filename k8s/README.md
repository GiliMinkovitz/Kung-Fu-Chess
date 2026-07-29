# Kung Fu Chess — Kubernetes (Phase 14.5 / 14.6)

Deploy **Gateway**, **Matchmaker**, **Game Server**, **Redis**, and **PostgreSQL** on Kubernetes with service discovery, health probes, and ConfigMap/Secret-based configuration.

```
                    Ingress (future)
                           |
                           v
                    gateway-service
                           |
              +------------+-------------+
              |                          |
              v                          v
     matchmaker-service          game-server-service
              |                          |
              +------------+-------------+
                           |
                           v
                    postgres-service
                           ^
                           |
                    redis-service
```

## Prerequisites

- A Kubernetes cluster (local: minikube, kind, Docker Desktop; or cloud)
- `kubectl` configured for your cluster
- Container images built and available to the cluster:
  - `kungfu-chess/gateway`
  - `kungfu-chess/matchmaker`
  - `kungfu-chess/game-server`

Build images from the repository root (same Dockerfiles as Compose):

```bash
docker build -f docker/Dockerfile.gateway -t kungfu-chess/gateway .
docker build -f docker/Dockerfile.matchmaker -t kungfu-chess/matchmaker .
docker build -f docker/Dockerfile.game_server -t kungfu-chess/game-server .
```

For local clusters, load images into the cluster (example with kind):

```bash
kind load docker-image kungfu-chess/gateway
kind load docker-image kungfu-chess/matchmaker
kind load docker-image kungfu-chess/game-server
```

## Before deploying

1. **Replace placeholder secrets** in `secrets.yaml` and `postgres/secret.yaml`:

   ```bash
   echo -n 'your-production-token' | base64
   echo -n 'your-postgres-password' | base64
   ```

   Update `KFC_INTERNAL_SERVICE_TOKEN` in `k8s/secrets.yaml` and `POSTGRES_PASSWORD` / `KFC_POSTGRES_PASSWORD` in `k8s/postgres/secret.yaml`. Committed placeholders decode to `REPLACE_ME`.

2. Apply manifests from the repository root:

   ```bash
   kubectl apply -f k8s/
   ```

   Kubernetes applies resources in filename order within each directory. The namespace and secrets are created first; PostgreSQL and Redis start before application pods become ready.

## Verify deployment

```bash
kubectl get pods -n kung-fu-chess
kubectl get services -n kung-fu-chess
kubectl get deployments -n kung-fu-chess
```

### Expected pods

| Deployment   | Replicas | Ready when |
|--------------|----------|------------|
| `postgres`   | 1        | `pg_isready -U kfc -d kfc` |
| `redis`      | 1        | Redis responds to `PING` |
| `game-server`| 1        | `/health`, `/ready`, PostgreSQL connected |
| `matchmaker` | 2        | Redis + game-server heartbeat + PostgreSQL |
| `gateway`    | 2        | Redis + PostgreSQL connected |

Startup order: **PostgreSQL → Redis → Game Server → Matchmaker → Gateway**.

Example healthy output:

```
NAME                           READY   STATUS    RESTARTS   AGE
gateway-xxxxxxxxxx-xxxxx       1/1     Running   0          2m
gateway-xxxxxxxxxx-xxxxx       1/1     Running   0          2m
game-server-xxxxxxxxxx-xxxxx   1/1     Running   0          3m
matchmaker-xxxxxxxxxx-xxxxx    1/1     Running   0          2m
matchmaker-xxxxxxxxxx-xxxxx    1/1     Running   0          2m
postgres-xxxxxxxxxx-xxxxx      1/1     Running   0          4m
redis-xxxxxxxxxx-xxxxx         1/1     Running   0          3m
```

## Service discovery

Inter-service URLs use Kubernetes DNS (ClusterIP services):

| Variable | Value |
|----------|-------|
| `KFC_REDIS_HOST` | `redis-service` |
| `KFC_MATCHMAKER_ENDPOINT` | `http://matchmaker-service:8770` |
| `KFC_GATEWAY_NOTIFICATION_ENDPOINT` | `http://gateway-service:8771` |
| `KFC_GAME_ENDPOINT` | `ws://game-server-service:8766` |
| `KFC_ALLOCATION_ENDPOINT` | `http://game-server-service:8767/allocate` |
| `KFC_POSTGRES_HOST` | `postgres-service` |
| `KFC_DATABASE_TYPE` | `postgres` |

All pods bind to `0.0.0.0` inside the container (`KFC_BIND_ADDRESS`).

## Database

Schema is applied on first PostgreSQL pod startup via `k8s/postgres/configmap.yaml` (mounted to `/docker-entrypoint-initdb.d`). Application pods do **not** run migrations.

Manual migration (if needed):

```bash
kubectl port-forward -n kung-fu-chess svc/postgres-service 5432:5432
psql -h localhost -U kfc -d kfc -f database/migrations/001_initial_schema.sql
```

## Health probes

| Service | Liveness | Readiness | Port |
|---------|----------|-----------|------|
| Gateway | `GET /health` | `GET /ready` | 8080 |
| Matchmaker | `GET /health` | `GET /ready` (Redis + game-server heartbeat) | 8772 |
| Game Server | `GET /health` | `GET /ready` | 8081 |
| Redis | `redis-cli ping` | `redis-cli ping` | 6379 |
| PostgreSQL | `pg_isready` | `pg_isready` | 5432 |

## Logs

```bash
# All pods in namespace
kubectl logs -n kung-fu-chess -l app.kubernetes.io/part-of=kung-fu-chess --tail=100

# Single deployment
kubectl logs -n kung-fu-chess deployment/gateway
kubectl logs -n kung-fu-chess deployment/matchmaker
kubectl logs -n kung-fu-chess deployment/game-server
kubectl logs -n kung-fu-chess deployment/redis

# Follow logs
kubectl logs -n kung-fu-chess deployment/gateway -f
```

## Port forwarding (local access)

Expose cluster services on localhost:

```bash
# Gateway WebSocket (client lobby)
kubectl port-forward -n kung-fu-chess svc/gateway-service 8765:8765

# Gateway health
kubectl port-forward -n kung-fu-chess svc/gateway-service 8080:8080

# Game WebSocket
kubectl port-forward -n kung-fu-chess svc/game-server-service 8766:8766

# Game health
kubectl port-forward -n kung-fu-chess svc/game-server-service 8081:8081

# Matchmaker API (debug)
kubectl port-forward -n kung-fu-chess svc/matchmaker-service 8770:8770

# Redis (debug)
kubectl port-forward -n kung-fu-chess svc/redis-service 6379:6379

# PostgreSQL (debug / migrations)
kubectl port-forward -n kung-fu-chess svc/postgres-service 5432:5432
```

Health checks via port-forward:

```bash
curl http://localhost:8080/health
curl http://localhost:8080/ready
curl http://localhost:8081/health
curl http://localhost:8081/ready
```

## Configuration

| Resource | Purpose |
|----------|---------|
| `gateway/configmap.yaml` | Gateway ports, endpoints, Redis, PostgreSQL |
| `matchmaker/configmap.yaml` | Matchmaker ports, gateway callback, allocation settings |
| `game-server/configmap.yaml` | Game ports, WebSocket/allocation URLs, PostgreSQL |
| `secrets.yaml` | `KFC_INTERNAL_SERVICE_TOKEN` (matchmaker and game-server) |
| `postgres/secret.yaml` | PostgreSQL passwords |
| `postgres/configmap.yaml` | Initial schema for Postgres init |

Edit a ConfigMap and roll out:

```bash
kubectl apply -f k8s/gateway/configmap.yaml
kubectl rollout restart -n kung-fu-chess deployment/gateway
```

## Scaling

Current replica counts (HPA not enabled yet):

| Deployment | Replicas |
|------------|----------|
| Gateway | 2 |
| Matchmaker | 2 |
| Game Server | 1 |
| Redis | 1 |
| PostgreSQL | 1 |

Manual scale (example):

```bash
kubectl scale -n kung-fu-chess deployment/gateway --replicas=2
```

## Teardown

```bash
kubectl delete -f k8s/
```

Or delete the namespace:

```bash
kubectl delete namespace kung-fu-chess
```

## Manifest layout

```
k8s/
├── namespace.yaml
├── secrets.yaml
├── postgres/
│   ├── deployment.yaml
│   ├── service.yaml
│   ├── secret.yaml
│   ├── pvc.yaml
│   └── configmap.yaml
├── redis/
│   ├── deployment.yaml
│   └── service.yaml
├── gateway/
│   ├── deployment.yaml
│   ├── service.yaml
│   └── configmap.yaml
├── matchmaker/
│   ├── deployment.yaml
│   ├── service.yaml
│   └── configmap.yaml
└── game-server/
    ├── deployment.yaml
    ├── service.yaml
    └── configmap.yaml
```

## Future phases

- **14.7** Observability — see [`docs/observability.md`](../docs/observability.md)
- **14.8** Horizontal Pod Autoscaling
- **14.9** Multi-region deployment
