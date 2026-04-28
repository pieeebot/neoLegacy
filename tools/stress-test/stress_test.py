"""
LCE Server Stress Testing Tool

Rapid bot connect/disconnect cycles to stress test thread safety
of the server's player list, socket handling, and movement processing.

Usage:
    python stress_test.py <host> <port> [options]

Options:
    --bots N          Max concurrent bots (default: 8)
    --cycles N        Number of connect/disconnect cycles (default: 50, 0=infinite)
    --hold MIN MAX    Hold time range in seconds before disconnect (default: 2 10)
    --ramp SECS       Delay between spawning each bot (default: 0.5)
    --move            Bots send movement packets while connected
    --burst N         Spawn N bots simultaneously (default: 1)
    --duration SECS   Run for N seconds then stop (default: 0, unlimited)
    --quiet           Suppress per-bot log messages
"""

import argparse
import logging
import math
import os
import random
import secrets
import socket
import struct
import sys
import threading
import time
from dataclasses import dataclass, field

# Import protocol code from the server-monitor tool.
# Search sibling tools directory first, then the itsRevela tools directory.
_TOOLS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
_SEARCH_PATHS = [
    os.path.join(_TOOLS, "server-monitor"),
    os.path.join(os.path.dirname(_TOOLS), "server-monitor"),
    os.path.normpath(os.path.join(_TOOLS, "..", "..", "..", "itsRevela", "tools", "server-monitor")),
]
for _p in _SEARCH_PATHS:
    if os.path.isdir(_p) and _p not in sys.path:
        sys.path.append(_p)
        break

from protocol import frame_packet, DataInputStream, DataOutputStream
from packets import (
    PRE_LOGIN, LOGIN, KEEP_ALIVE, CUSTOM_PAYLOAD, DISCONNECT,
    build_prelogin, build_login, build_keep_alive,
    build_custom_payload, parse_prelogin_response,
    parse_login_response_stream, DISCONNECT_REASONS,
)

logger = logging.getLogger("stress")

SMALLID_REJECT = 0xFF

CIPHER_KEY_PREFIX = (
    b"\xFA\x00\x07"
    b"\x00\x4D\x00\x43\x00\x7C\x00\x43\x00\x4B\x00\x65\x00\x79"
    b"\x00\x20"
)
CIPHER_KEY_TOTAL = len(CIPHER_KEY_PREFIX) + 32

CIPHER_ON_PATTERN = (
    b"\xFA\x00\x06"
    b"\x00\x4D\x00\x43\x00\x7C\x00\x43\x00\x4F\x00\x6E"
    b"\x00\x00"
)

CIPHER_KEY_CHANNEL = "MC|CKey"
CIPHER_ACK_CHANNEL = "MC|CAck"
CIPHER_ON_CHANNEL = "MC|COn"
IDENTITY_TOKEN_ISSUE = "MC|CTIssue"
IDENTITY_TOKEN_CHALLENGE = "MC|CTChallenge"
IDENTITY_TOKEN_RESPONSE = "MC|CTResponse"


def _build_channel_pattern(channel: str) -> bytes:
    result = b"\xFA"
    result += struct.pack(">h", len(channel))
    for ch in channel:
        result += struct.pack(">H", ord(ch))
    return result


# ---------------------------------------------------------------------------
# CipherState (AES-CTR, optional dependency)
# ---------------------------------------------------------------------------

class CipherState:
    def __init__(self, key: bytes, iv: bytes):
        from Crypto.Cipher import AES
        from Crypto.Util import Counter
        ctr = Counter.new(128, initial_value=int.from_bytes(iv, "big"))
        self._cipher = AES.new(key, AES.MODE_CTR, counter=ctr)

    def process(self, data: bytes) -> bytes:
        return self._cipher.encrypt(data)


# ---------------------------------------------------------------------------
# Statistics tracker
# ---------------------------------------------------------------------------

@dataclass
class Stats:
    lock: threading.Lock = field(default_factory=threading.Lock)
    connects: int = 0
    disconnects: int = 0
    rejections: int = 0
    errors: int = 0
    moves_sent: int = 0
    keepalives_sent: int = 0
    start_time: float = 0.0

    def summary(self) -> str:
        elapsed = time.time() - self.start_time if self.start_time else 0
        with self.lock:
            return (
                f"[{elapsed:.0f}s] "
                f"connects={self.connects} "
                f"disconnects={self.disconnects} "
                f"rejections={self.rejections} "
                f"errors={self.errors} "
                f"moves={self.moves_sent} "
                f"keepalives={self.keepalives_sent}"
            )


# ---------------------------------------------------------------------------
# Movement packet builder
# ---------------------------------------------------------------------------

MOVE_PLAYER = 0x0D  # MovePlayerPacket::PosRot — what we send AND what server sends for teleports.

def build_move_player(x: float, y: float, z: float,
                      yaw: float, pitch: float, on_ground: bool) -> bytes:
    # Wire order matches MovePlayerPacket::PosRot::write: x, y (feet),
    # yView (eye), z, yaw, pitch, flags. Server kicks for IllegalStance
    # if (yView - y) is outside [0.1, 1.65], so feet must come first.
    dos = DataOutputStream()
    dos.write_double(x)
    dos.write_double(y)
    dos.write_double(y + 1.62)
    dos.write_double(z)
    dos.write_float(yaw)
    dos.write_float(pitch)
    dos.write_bool(on_ground)
    return dos.getvalue()


# ---------------------------------------------------------------------------
# Single bot stress connection
# ---------------------------------------------------------------------------

class StressBot:
    """A bot that connects, optionally moves around, then disconnects."""

    def __init__(self, name: str, host: str, port: int, xuid: int,
                 hold_min: float, hold_max: float, send_moves: bool,
                 stats: Stats, quiet: bool):
        self.name = name
        self.host = host
        self.port = port
        self.xuid = xuid
        self.hold_min = hold_min
        self.hold_max = hold_max
        self.send_moves = send_moves
        self.stats = stats
        self.quiet = quiet
        self._sock: socket.socket | None = None
        self._recv_buf = bytearray()
        self._scan_buf = bytearray()
        self._recv_cipher: CipherState | None = None
        self._send_cipher: CipherState | None = None
        self._cipher_key = b""
        self._cipher_iv = b""
        self._identity_token = b""
        self._entity_id = 0
        self._running = True
        # Server-tracked position. Initialized when server sends its first
        # MovePlayer::PosRot teleport after login, and updated whenever the
        # server teleports us (eg. plugin scatter, anti-cheat correction).
        self._pos_x = 0.0
        self._pos_y = 64.0
        self._pos_z = 0.0
        self._pos_initialized = False

    def log(self, msg: str) -> None:
        if not self.quiet:
            logger.info(msg)

    def run(self) -> bool:
        """Run one connect-hold-disconnect cycle. Returns True on success."""
        try:
            return self._do_cycle()
        except Exception as e:
            self.log(f"[{self.name}] error: {e}")
            with self.stats.lock:
                self.stats.errors += 1
            return False
        finally:
            if self._sock:
                try:
                    self._sock.close()
                except OSError:
                    pass
                self._sock = None

    def _do_cycle(self) -> bool:
        # Connect
        self._sock = socket.create_connection((self.host, self.port), timeout=10)
        self._sock.settimeout(5.0)

        # SmallID
        first_byte = self._recv_exact(1)
        if first_byte[0] == SMALLID_REJECT:
            reject_data = self._recv_exact(5)
            reason_id = struct.unpack(">i", reject_data[1:5])[0]
            reason = DISCONNECT_REASONS.get(reason_id, f"Unknown({reason_id})")
            self.log(f"[{self.name}] rejected: {reason}")
            with self.stats.lock:
                self.stats.rejections += 1
            return False

        small_id = first_byte[0]
        self.log(f"[{self.name}] assigned smallId={small_id}")

        # PreLogin
        self._send_packet(PRE_LOGIN, build_prelogin(self.name))

        # Read PreLogin response
        self._sock.settimeout(5.0)
        if not self._read_until_packet(PRE_LOGIN, timeout=5.0):
            self.log(f"[{self.name}] no PreLogin response")
            with self.stats.lock:
                self.stats.errors += 1
            return False

        # Login
        self._send_packet(LOGIN, build_login(self.name, self.xuid))

        # Read Login response
        if not self._read_until_packet(LOGIN, timeout=5.0):
            self.log(f"[{self.name}] no Login response")
            with self.stats.lock:
                self.stats.errors += 1
            return False

        with self.stats.lock:
            self.stats.connects += 1

        self.log(f"[{self.name}] connected (entityId={self._entity_id})")

        # Cipher scan (3 second window)
        self._sock.settimeout(1.0)
        self._do_cipher_scan()

        # Hold phase: stay connected, send keepalives and optional movement
        hold_time = random.uniform(self.hold_min, self.hold_max)
        hold_end = time.time() + hold_time
        last_keepalive = time.time()
        keepalive_counter = 0

        while time.time() < hold_end and self._running:
            # Drain incoming data
            try:
                chunk = self._sock.recv(65536)
                if not chunk:
                    break
                self._recv_buf.extend(chunk)
                self._drain_frames()
            except socket.timeout:
                pass
            except OSError:
                break

            now = time.time()

            # Keepalive every 10s
            if now - last_keepalive >= 10.0:
                keepalive_counter += 1
                self._send_packet(KEEP_ALIVE, build_keep_alive(keepalive_counter))
                with self.stats.lock:
                    self.stats.keepalives_sent += 1
                last_keepalive = now

            # Movement packets every 50ms. We can't do real travel because
            # the server's anti-cheat compares our claimed position against
            # what its own physics computes, and we don't simulate collision
            # or gravity. Instead we drift ±0.3 blocks from whatever
            # position the server most recently teleported us to. To spread
            # bots out, use the test plugin's /fktest scatter from in-game.
            if self.send_moves and self._pos_initialized:
                new_x = self._pos_x + random.uniform(-0.3, 0.3)
                new_z = self._pos_z + random.uniform(-0.3, 0.3)
                yaw = random.uniform(0, 360)
                self._send_packet(MOVE_PLAYER,
                    build_move_player(new_x, self._pos_y, new_z, yaw, 0.0, True))
                # Optimistically update; server will correct us via PosRot
                # if it disagreed (eg. we drifted into a block).
                self._pos_x = new_x
                self._pos_z = new_z
                with self.stats.lock:
                    self.stats.moves_sent += 1
                time.sleep(0.05)

        # Disconnect (just close the socket)
        self.log(f"[{self.name}] disconnecting after {hold_time:.1f}s hold")
        with self.stats.lock:
            self.stats.disconnects += 1
        return True

    def _do_cipher_scan(self) -> None:
        """Wait for the cipher handshake to finish or up to ~4s.

        Returns early once both keys are exchanged. The upper bound has to
        cover the worst case where a stack of plaintext setup packets
        (level info, scoreboard, initial chunks) sits in front of MC|CKey
        in the recv buffer. The server's own cipher-handshake grace is
        100 ticks (~5s).
        """
        scan_start = time.time()
        scan_buf = bytearray()

        while time.time() - scan_start < 4.0 and self._running:
            # _handle_custom_payload may have already activated cipher via
            # the drain path inside _read_until_packet.
            if self._cipher_key and self._recv_cipher:
                return

            try:
                chunk = self._sock.recv(65536)
                if not chunk:
                    return
                self._recv_buf.extend(chunk)
            except socket.timeout:
                pass
            except OSError:
                return

            # Also drain framed packets
            self._drain_frames()

            # Accumulate raw data for cipher pattern scanning
            scan_buf.extend(self._recv_buf)

            # Look for MC|CKey
            if not self._cipher_key:
                idx = scan_buf.find(CIPHER_KEY_PREFIX)
                if idx >= 0 and idx + CIPHER_KEY_TOTAL <= len(scan_buf):
                    key_start = idx + len(CIPHER_KEY_PREFIX)
                    key_data = bytes(scan_buf[key_start:key_start + 32])
                    self._cipher_key = key_data[:16]
                    self._cipher_iv = key_data[16:32]
                    self.log(f"[{self.name}] got cipher key")

                    self._send_packet(CUSTOM_PAYLOAD,
                        build_custom_payload(CIPHER_ACK_CHANNEL))
                    iv_send = bytearray(self._cipher_iv)
                    iv_send[0] ^= 0x80
                    self._send_cipher = CipherState(self._cipher_key, bytes(iv_send))

                    del scan_buf[:idx + CIPHER_KEY_TOTAL]

            # Look for MC|COn
            if self._cipher_key and not self._recv_cipher:
                idx = scan_buf.find(CIPHER_ON_PATTERN)
                if idx >= 0:
                    self._recv_cipher = CipherState(self._cipher_key, self._cipher_iv)
                    self.log(f"[{self.name}] cipher active")
                    return

        if not self._cipher_key:
            self.log(f"[{self.name}] no cipher (server has it disabled)")

    def _read_until_packet(self, expected_id: int, timeout: float) -> bool:
        """Read frames until we get the expected packet ID or timeout."""
        deadline = time.time() + timeout
        while time.time() < deadline:
            try:
                chunk = self._sock.recv(65536)
                if not chunk:
                    return False
                self._recv_buf.extend(chunk)
            except socket.timeout:
                continue
            except OSError:
                return False

            while len(self._recv_buf) >= 4:
                payload_len = struct.unpack(">I", self._recv_buf[:4])[0]
                total = 4 + payload_len
                if len(self._recv_buf) < total:
                    break

                raw_payload = bytes(self._recv_buf[4:total])
                del self._recv_buf[:total]

                if self._recv_cipher:
                    raw_payload = self._recv_cipher.process(raw_payload)

                if not raw_payload:
                    continue

                packet_id = raw_payload[0]
                data = raw_payload[1:]

                if packet_id == expected_id:
                    if packet_id == PRE_LOGIN:
                        try:
                            parsed = parse_prelogin_response(data)
                            self.log(f"[{self.name}] PreLogin: {parsed['player_count']} online")
                        except Exception:
                            pass
                        return True
                    elif packet_id == LOGIN:
                        try:
                            dis = DataInputStream(data)
                            parsed = parse_login_response_stream(dis)
                            self._entity_id = parsed["entity_id"]
                        except Exception:
                            pass
                        return True

                # Handle cipher handshake during the login wait. With
                # require-secure-client, the server holds the Login response
                # behind the security gate until cipher activates, so the
                # gate-bypass MC|CKey/MC|COn frames arrive before LOGIN.
                # Dropping them here would deadlock both sides.
                elif packet_id == CUSTOM_PAYLOAD:
                    self._handle_custom_payload(data)

                elif packet_id == DISCONNECT:
                    try:
                        dis = DataInputStream(data)
                        reason_id = dis.read_int()
                        reason = DISCONNECT_REASONS.get(reason_id, f"Unknown({reason_id})")
                        self.log(f"[{self.name}] kicked during login: {reason}")
                    except Exception:
                        pass
                    return False

        return False

    def _drain_frames(self) -> None:
        """Drain and discard all complete frames from recv buffer."""
        while len(self._recv_buf) >= 4:
            payload_len = struct.unpack(">I", self._recv_buf[:4])[0]
            total = 4 + payload_len
            if len(self._recv_buf) < total:
                break

            raw_payload = bytes(self._recv_buf[4:total])
            del self._recv_buf[:total]

            if self._recv_cipher:
                raw_payload = self._recv_cipher.process(raw_payload)

            if not raw_payload:
                continue

            packet_id = raw_payload[0]
            data = raw_payload[1:]

            # Handle identity tokens
            if packet_id == CUSTOM_PAYLOAD:
                self._handle_custom_payload(data)
            elif packet_id == MOVE_PLAYER:
                self._handle_server_move(data)

    def _handle_server_move(self, data: bytes) -> None:
        """Track server's view of our position. PosRot format:
        double x, double y, double yView, double z, float yRot, float xRot, byte flags."""
        try:
            dis = DataInputStream(data)
            x = dis.read_double()
            y = dis.read_double()
            _yView = dis.read_double()
            z = dis.read_double()
            self._pos_x = x
            self._pos_y = y
            self._pos_z = z
            self._pos_initialized = True
        except Exception:
            pass

    def _handle_custom_payload(self, data: bytes) -> None:
        """Handle cipher handshake and identity token channels."""
        try:
            dis = DataInputStream(data)
            channel = dis.read_utf()
            length = dis.read_short()
            payload = dis.read_raw(length) if length > 0 else b""

            # Cipher channels arrive in plaintext before encryption is active.
            # Handle them here so the bot survives bursts where the whole
            # handshake frame lands in a single recv(), bypassing the leftover
            # byte-pattern scan.
            if channel == CIPHER_KEY_CHANNEL and len(payload) == 32 and not self._cipher_key:
                self._cipher_key = payload[:16]
                self._cipher_iv = payload[16:32]
                self.log(f"[{self.name}] got cipher key")
                self._send_packet(CUSTOM_PAYLOAD,
                    build_custom_payload(CIPHER_ACK_CHANNEL))
                iv_send = bytearray(self._cipher_iv)
                iv_send[0] ^= 0x80
                self._send_cipher = CipherState(self._cipher_key, bytes(iv_send))
            elif channel == CIPHER_ON_CHANNEL:
                if self._cipher_key and not self._recv_cipher:
                    self._recv_cipher = CipherState(self._cipher_key, self._cipher_iv)
                    self.log(f"[{self.name}] cipher active")
            elif channel == IDENTITY_TOKEN_ISSUE and len(payload) == 32:
                self._identity_token = payload
                self.log(f"[{self.name}] got identity token")
            elif channel == IDENTITY_TOKEN_CHALLENGE:
                if self._identity_token and len(self._identity_token) == 32:
                    self._send_packet(CUSTOM_PAYLOAD,
                        build_custom_payload(IDENTITY_TOKEN_RESPONSE, self._identity_token))
                else:
                    self._send_packet(CUSTOM_PAYLOAD,
                        build_custom_payload(IDENTITY_TOKEN_RESPONSE))
                self.log(f"[{self.name}] answered identity challenge")
        except Exception:
            pass

    def _send_packet(self, packet_id: int, payload: bytes) -> None:
        raw = frame_packet(packet_id, payload)
        if self._send_cipher:
            header = raw[:4]
            encrypted = self._send_cipher.process(raw[4:])
            raw = header + encrypted
        try:
            if self._sock:
                self._sock.sendall(raw)
        except OSError:
            pass

    def _recv_exact(self, n: int) -> bytes:
        data = b""
        while len(data) < n:
            chunk = self._sock.recv(n - len(data))
            if not chunk:
                raise ConnectionError("Connection closed during recv")
            data += chunk
        return data


# ---------------------------------------------------------------------------
# Orchestrator
# ---------------------------------------------------------------------------

class StressTestRunner:
    def __init__(self, host: str, port: int, max_bots: int, cycles: int,
                 hold_min: float, hold_max: float, ramp: float,
                 send_moves: bool, burst: int, duration: float,
                 quiet: bool):
        self.host = host
        self.port = port
        self.max_bots = max_bots
        self.cycles = cycles
        self.hold_min = hold_min
        self.hold_max = hold_max
        self.ramp = ramp
        self.send_moves = send_moves
        self.burst = burst
        self.duration = duration
        self.quiet = quiet
        self.stats = Stats()
        self._active = threading.Semaphore(max_bots)
        self._stop = threading.Event()
        self._bot_counter = 0
        self._counter_lock = threading.Lock()

    def run(self) -> None:
        self.stats.start_time = time.time()
        logger.info(f"Stress test: {self.host}:{self.port}")
        logger.info(f"  max_bots={self.max_bots} cycles={self.cycles or 'infinite'} "
                     f"hold={self.hold_min}-{self.hold_max}s ramp={self.ramp}s "
                     f"burst={self.burst} moves={self.send_moves}")
        if self.duration:
            logger.info(f"  duration={self.duration}s")

        # Status printer
        status_thread = threading.Thread(target=self._print_status, daemon=True)
        status_thread.start()

        # Duration timer
        if self.duration > 0:
            timer = threading.Timer(self.duration, self._stop.set)
            timer.daemon = True
            timer.start()

        cycle = 0
        try:
            while not self._stop.is_set():
                if self.cycles > 0 and cycle >= self.cycles:
                    break

                # Spawn a burst of bots
                threads = []
                for _ in range(self.burst):
                    if self._stop.is_set():
                        break
                    if self.cycles > 0 and cycle >= self.cycles:
                        break

                    self._active.acquire()
                    if self._stop.is_set():
                        self._active.release()
                        break

                    with self._counter_lock:
                        self._bot_counter += 1
                        bot_name = f"StressBot{self._bot_counter}"

                    xuid = secrets.randbits(62) | (1 << 32)
                    bot = StressBot(
                        name=bot_name,
                        host=self.host,
                        port=self.port,
                        xuid=xuid,
                        hold_min=self.hold_min,
                        hold_max=self.hold_max,
                        send_moves=self.send_moves,
                        stats=self.stats,
                        quiet=self.quiet,
                    )

                    t = threading.Thread(
                        target=self._run_bot, args=(bot,), daemon=True)
                    t.start()
                    threads.append(t)
                    cycle += 1

                # Ramp delay between bursts
                if self.ramp > 0 and not self._stop.is_set():
                    self._stop.wait(self.ramp)

        except KeyboardInterrupt:
            logger.info("\nInterrupted by user")
            self._stop.set()

        # Wait for active bots to finish
        logger.info("Waiting for active bots to finish...")
        for _ in range(self.max_bots):
            self._active.acquire(timeout=30)

        logger.info(f"\nFinal: {self.stats.summary()}")

    def _run_bot(self, bot: StressBot) -> None:
        try:
            bot.run()
        finally:
            self._active.release()

    def _print_status(self) -> None:
        while not self._stop.is_set():
            self._stop.wait(5.0)
            if not self._stop.is_set():
                logger.info(self.stats.summary())


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="LCE Server Stress Testing Tool",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("host", nargs="?", default="127.0.0.1",
                        help="Server hostname or IP (default: 127.0.0.1)")
    parser.add_argument("port", nargs="?", type=int, default=19132,
                        help="Server port (default: 19132)")
    parser.add_argument("--bots", type=int, default=8,
                        help="Max concurrent bots (default: 8)")
    parser.add_argument("--cycles", type=int, default=50,
                        help="Connect/disconnect cycles (default: 50, 0=infinite)")
    parser.add_argument("--hold", type=float, nargs=2, default=[2.0, 10.0],
                        metavar=("MIN", "MAX"),
                        help="Hold time range in seconds (default: 2 10)")
    parser.add_argument("--ramp", type=float, default=0.5,
                        help="Delay between spawning bots (default: 0.5)")
    parser.add_argument("--move", action="store_true",
                        help="Bots send movement packets while connected")
    parser.add_argument("--burst", type=int, default=1,
                        help="Bots to spawn simultaneously (default: 1)")
    parser.add_argument("--duration", type=float, default=0,
                        help="Run for N seconds then stop (default: unlimited)")
    parser.add_argument("--quiet", action="store_true",
                        help="Suppress per-bot log messages")

    args = parser.parse_args()

    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s %(message)s",
        datefmt="%H:%M:%S",
    )

    runner = StressTestRunner(
        host=args.host,
        port=args.port,
        max_bots=args.bots,
        cycles=args.cycles,
        hold_min=args.hold[0],
        hold_max=args.hold[1],
        ramp=args.ramp,
        send_moves=args.move,
        burst=args.burst,
        duration=args.duration,
        quiet=args.quiet,
    )
    runner.run()


if __name__ == "__main__":
    main()
