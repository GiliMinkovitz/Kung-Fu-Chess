# Stage 1: build KungFuChessServer with CMake
FROM debian:bookworm-slim AS build

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    libboost-system-dev \
    libpq-dev \
    libsqlite3-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --target KungFuChessServer -j"$(nproc)"

# Stage 2: minimal runtime image
FROM debian:bookworm-slim AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    libboost-system1.74.0 \
    libpq5 \
    libsqlite3-0 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=build /src/build/KungFuChessServer /app/KungFuChessServer

EXPOSE 8765

CMD ["./KungFuChessServer"]
