#!/usr/bin/env python3
"""End-to-end matchmaking smoke test for Docker Compose and Kubernetes.

Works against services exposed on localhost (Compose port mappings or kubectl
port-forward). Validates login, queue, match, and game_redirect for two players.

Environment variables:
  KFC_GATEWAY_URL     Gateway WebSocket URL (default: ws://localhost:8765)
  KFC_GAME_URL        Expected game_redirect endpoint (default: ws://localhost:8766)
  KFC_SMOKE_PASSWORD  Password for auto-registering smoke users
  KFC_SMOKE_TIMEOUT   Seconds to wait for match (default: 60)
  KFC_SMOKE_VERIFY_JOIN  When "1", fail unless join_game returns join_ok
"""

from __future__ import annotations

import asyncio
import os
import sys
import uuid

try:
    import websockets
except ImportError:
    print(
        "Missing dependency: pip install websockets",
        file=sys.stderr,
    )
    sys.exit(2)

GATEWAY_URL = os.environ.get("KFC_GATEWAY_URL", "ws://localhost:8765")
EXPECTED_GAME_URL = os.environ.get("KFC_GAME_URL", "ws://localhost:8766")
PASSWORD = os.environ.get("KFC_SMOKE_PASSWORD", "smoke-test-password")
TIMEOUT = float(os.environ.get("KFC_SMOKE_TIMEOUT", "60"))
VERIFY_JOIN = os.environ.get("KFC_SMOKE_VERIFY_JOIN", "0") == "1"


def normalize_ws_url(url: str) -> str:
    return url.replace("127.0.0.1", "localhost")


async def recv_line(ws: websockets.ClientConnection, timeout: float) -> str:
    raw = await asyncio.wait_for(ws.recv(), timeout=timeout)
    return str(raw).strip().split("\n", 1)[0]


async def login_and_queue(
    ws: websockets.ClientConnection, username: str, timeout: float
) -> None:
    await ws.send(f"login {username} {PASSWORD}")
    message = await recv_line(ws, timeout)
    if not message.startswith("login_ok"):
        raise RuntimeError(f"{username}: expected login_ok, got {message!r}")

    await ws.send("play")


async def wait_for_redirect(
    ws: websockets.ClientConnection, username: str, timeout: float
) -> tuple[str, int, str]:
    loop = asyncio.get_running_loop()
    deadline = loop.time() + timeout
    while True:
        remaining = deadline - loop.time()
        if remaining <= 0:
            raise TimeoutError(f"{username}: timed out waiting for game_redirect")
        message = await recv_line(ws, min(remaining, 5.0))
        if message == "search_failed":
            raise RuntimeError(f"{username}: matchmaker rejected play request")
        if message.startswith("game_redirect "):
            parts = message.split()
            if len(parts) != 4:
                raise RuntimeError(f"{username}: invalid game_redirect: {message!r}")
            return parts[1], int(parts[2]), parts[3]


async def player_flow(
    gateway_url: str,
    username: str,
    timeout: float,
) -> tuple[str, int, str]:
    async with websockets.connect(gateway_url, open_timeout=timeout) as ws:
        await login_and_queue(ws, username, timeout)
        return await wait_for_redirect(ws, username, timeout)


async def try_join_game(endpoint: str, room_id: int, timeout: float) -> str:
    async with websockets.connect(endpoint, open_timeout=timeout) as ws:
        await ws.send(f"join_game {room_id}")
        return await recv_line(ws, timeout)


async def try_join_game_with_retry(
    endpoint: str, room_id: int, timeout: float, attempts: int = 5
) -> str:
    last_message = ""
    for attempt in range(attempts):
        if attempt > 0:
            await asyncio.sleep(0.5)
        last_message = await try_join_game(endpoint, room_id, timeout)
        if last_message.startswith("join_ok"):
            return last_message
        if last_message != "join_failed room_not_found":
            return last_message
    return last_message


async def run_smoke_test() -> None:
    suffix = uuid.uuid4().hex[:8]
    white_user = f"smoke_{suffix}_w"
    black_user = f"smoke_{suffix}_b"

    print(f"Gateway: {GATEWAY_URL}")
    print(f"Expected game endpoint: {EXPECTED_GAME_URL}")

    white_task = asyncio.create_task(player_flow(GATEWAY_URL, white_user, TIMEOUT))
    await asyncio.sleep(0.5)
    black_task = asyncio.create_task(player_flow(GATEWAY_URL, black_user, TIMEOUT))

    white_endpoint, white_room, white_side = await white_task
    black_endpoint, black_room, black_side = await black_task

    if white_room != black_room:
        raise RuntimeError(f"room mismatch: {white_room} vs {black_room}")

    sides = {white_side, black_side}
    if sides != {"white", "black"}:
        raise RuntimeError(f"unexpected sides: {white_side}, {black_side}")

    expected = normalize_ws_url(EXPECTED_GAME_URL)
    for label, endpoint in (("white", white_endpoint), ("black", black_endpoint)):
        if normalize_ws_url(endpoint) != expected:
            raise RuntimeError(
                f"{label}: unexpected game endpoint {endpoint!r} (expected {EXPECTED_GAME_URL!r})"
            )

    print(f"Matched room_id={white_room} endpoint={white_endpoint} sides={white_side}/{black_side}")

    join_message = await try_join_game_with_retry(white_endpoint, white_room, min(TIMEOUT, 15.0))
    print(f"join_game response: {join_message}")

    if join_message.startswith("join_ok"):
        print("OK: matchmaking smoke test passed (including join_game)")
        return

    if VERIFY_JOIN:
        raise RuntimeError(f"join_game failed: {join_message!r}")

    print(
        "OK: matchmaking smoke test passed "
        f"(join_game returned {join_message!r}; set KFC_SMOKE_VERIFY_JOIN=1 to require join_ok)"
    )


def main() -> int:
    try:
        asyncio.run(run_smoke_test())
    except Exception as exc:  # noqa: BLE001 - smoke test reports any failure
        print(f"FAIL: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
