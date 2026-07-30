#!/usr/bin/env python3
"""
Gateway-level matchmaking smoke test for the Docker Compose multiplayer stack.

This script connects two real WebSocket clients to the public Gateway endpoint,
logs in, enters matchmaking, and verifies both players receive match assignment
messages (match_found, game_start, game_redirect).

It intentionally does not join the game server because distributed game-server
authentication is not part of this test.

Prerequisites:
  docker compose up   (gateway on ws://localhost:8765)

Usage:
  pip install -r tests/smoke/requirements.txt
  python tests/smoke/matchmaking_smoke.py

Environment variables:
  KFC_GATEWAY_WS       Gateway WebSocket URL (default: ws://localhost:8765)
  KFC_MATCH_TIMEOUT_S  Seconds to wait for matchmaking (default: 15)
  KFC_SMOKE_PASSWORD   Login password (default: smoke-test-pass)
"""

from __future__ import annotations

import asyncio
import os
import sys
import uuid
from dataclasses import dataclass, field
from typing import Optional

try:
    import websockets
    from websockets.exceptions import ConnectionClosed
except ImportError:
    print(
        "Missing dependency: websockets\n"
        "Install with: pip install -r tests/smoke/requirements.txt",
        file=sys.stderr,
    )
    sys.exit(1)


DEFAULT_GATEWAY_WS = "ws://localhost:8765"
DEFAULT_PASSWORD = "smoke-test-pass"
DEFAULT_MATCH_TIMEOUT_S = 15.0
CONNECT_TIMEOUT_S = 10.0


@dataclass
class MatchOutcome:
    side: Optional[str] = None
    room_id: Optional[str] = None
    endpoint: Optional[str] = None
    saw_match_found: bool = False
    saw_game_start: bool = False
    saw_game_redirect: bool = False


@dataclass
class PlayerOutcome:
    name: str
    messages: list[str] = field(default_factory=list)
    match: MatchOutcome = field(default_factory=MatchOutcome)
    error: Optional[str] = None


def first_line(message: str) -> str:
    line = message.split("\n", 1)[0]
    return line.strip()


def apply_message(line: str, match: MatchOutcome) -> None:
    tokens = line.split()
    if not tokens:
        return

    command = tokens[0]

    if command == "match_found" and len(tokens) >= 2:
        match.saw_match_found = True
        match.side = match.side or tokens[1]
        return

    if command == "game_start" and len(tokens) >= 2:
        match.saw_game_start = True
        match.side = match.side or tokens[1]
        return

    if command == "game_redirect" and len(tokens) == 4:
        match.saw_game_redirect = True
        match.endpoint = tokens[1]
        match.room_id = tokens[2]
        match.side = tokens[3]


def match_complete(match: MatchOutcome) -> bool:
    return match.saw_match_found and match.saw_game_start and match.saw_game_redirect


async def player_session(
    name: str,
    gateway_ws: str,
    password: str,
    play_delay_s: float,
    match_timeout_s: float,
) -> PlayerOutcome:
    outcome = PlayerOutcome(name=name)
    loop = asyncio.get_running_loop()
    deadline = loop.time() + match_timeout_s

    try:
        async with websockets.connect(
            gateway_ws,
            open_timeout=CONNECT_TIMEOUT_S,
            close_timeout=2,
        ) as ws:
            await ws.send(f"login {name} {password}")

            while True:
                remaining = deadline - loop.time()
                if remaining <= 0:
                    outcome.error = "timeout waiting for login_ok"
                    return outcome

                raw = await asyncio.wait_for(ws.recv(), timeout=remaining)
                line = first_line(raw)
                outcome.messages.append(line)

                if line.startswith("login_ok"):
                    break
                if line.startswith("login_failed"):
                    outcome.error = f"login rejected: {line}"
                    return outcome

            if play_delay_s > 0:
                await asyncio.sleep(play_delay_s)

            await ws.send("play")

            while not match_complete(outcome.match):
                remaining = deadline - loop.time()
                if remaining <= 0:
                    outcome.error = "timeout waiting for matchmaking completion"
                    return outcome

                raw = await asyncio.wait_for(ws.recv(), timeout=remaining)
                line = first_line(raw)
                outcome.messages.append(line)
                apply_message(line, outcome.match)

    except asyncio.TimeoutError:
        if outcome.error is None:
            outcome.error = "timeout waiting for gateway messages"
    except ConnectionClosed as exc:
        outcome.error = f"websocket closed unexpectedly: {exc}"
    except OSError as exc:
        outcome.error = f"connection error: {exc}"
    except Exception as exc:  # noqa: BLE001 - smoke test reports any failure
        outcome.error = f"unexpected error: {exc}"

    if outcome.error is None and not match_complete(outcome.match):
        missing = []
        if not outcome.match.saw_match_found:
            missing.append("match_found")
        if not outcome.match.saw_game_start:
            missing.append("game_start")
        if not outcome.match.saw_game_redirect:
            missing.append("game_redirect")
        outcome.error = "incomplete matchmaking: missing " + ", ".join(missing)

    return outcome


def validate_outcomes(p1: PlayerOutcome, p2: PlayerOutcome) -> Optional[str]:
    for player in (p1, p2):
        if player.error:
            return f"{player.name}: {player.error}"

    sides = {p1.match.side, p2.match.side}
    if sides != {"white", "black"}:
        return (
            f"expected one white and one black side, got "
            f"{p1.name}={p1.match.side!r}, {p2.name}={p2.match.side!r}"
        )

    if p1.match.room_id != p2.match.room_id:
        return (
            f"room_id mismatch: {p1.name}={p1.match.room_id!r}, "
            f"{p2.name}={p2.match.room_id!r}"
        )

    if not p1.match.room_id:
        return "room_id missing from game_redirect"

    return None


def print_failure_diagnostics(p1: PlayerOutcome, p2: PlayerOutcome, reason: str) -> None:
    print(f"FAIL: {reason}", file=sys.stderr)
    for player in (p1, p2):
        print(f"\n--- {player.name} ---", file=sys.stderr)
        if player.error:
            print(f"error: {player.error}", file=sys.stderr)
        print("messages received:", file=sys.stderr)
        if player.messages:
            for index, message in enumerate(player.messages, start=1):
                print(f"  {index}. {message}", file=sys.stderr)
        else:
            print("  (none)", file=sys.stderr)
        match = player.match
        print(
            "parsed: "
            f"match_found={match.saw_match_found}, "
            f"game_start={match.saw_game_start}, "
            f"game_redirect={match.saw_game_redirect}, "
            f"side={match.side!r}, room_id={match.room_id!r}, endpoint={match.endpoint!r}",
            file=sys.stderr,
        )


async def run_smoke_test() -> int:
    run_id = uuid.uuid4().hex[:8]
    p1_name = f"smoke_p1_{run_id}"
    p2_name = f"smoke_p2_{run_id}"

    gateway_ws = os.environ.get("KFC_GATEWAY_WS", DEFAULT_GATEWAY_WS)
    password = os.environ.get("KFC_SMOKE_PASSWORD", DEFAULT_PASSWORD)
    match_timeout_s = float(os.environ.get("KFC_MATCH_TIMEOUT_S", DEFAULT_MATCH_TIMEOUT_S))

    print(f"Gateway: {gateway_ws}")
    print(f"Players: {p1_name}, {p2_name}")
    print(f"Match timeout: {match_timeout_s}s")

    p1, p2 = await asyncio.gather(
        player_session(p1_name, gateway_ws, password, play_delay_s=0.0, match_timeout_s=match_timeout_s),
        player_session(p2_name, gateway_ws, password, play_delay_s=0.2, match_timeout_s=match_timeout_s),
    )

    failure = validate_outcomes(p1, p2)
    if failure:
        print_failure_diagnostics(p1, p2, failure)
        return 1

    print("PASS: both players matched and received game_redirect")
    print(f"  room_id={p1.match.room_id}")
    print(f"  endpoint={p1.match.endpoint}")
    print(f"  sides: {p1.name}={p1.match.side}, {p2.name}={p2.match.side}")
    return 0


def main() -> None:
    sys.exit(asyncio.run(run_smoke_test()))


if __name__ == "__main__":
    main()
