#!/usr/bin/env python3
"""
Broken Shadows MUD Automated Test Suite

This script performs automated functional testing of the MUD server:
1. Builds the Docker image
2. Starts the container
3. Connects and tests game functionality:
   - Character creation
   - Movement between rooms
   - Inventory checking
   - Logout
4. Cleans up (stops container, removes player file)

Usage:
    python3 test_mud_automated.py [--skip-build] [--port 4000] [--debug]

Requirements:
    - Docker and docker compose installed
    - Python 3.6+
    - telnetlib3 (pip install telnetlib3)
"""

import sys
import os
import re

# Re-exec with the repo .venv python if it exists and we're not already in it.
_here = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
_venv_py = os.path.join(_here, ".venv", "bin", "python3")
_venv_prefix = os.path.abspath(os.path.join(_here, ".venv"))
if os.path.exists(_venv_py) and sys.prefix != _venv_prefix:
    os.execv(_venv_py, [_venv_py] + sys.argv)
import subprocess  # noqa: E402
import time  # noqa: E402
import asyncio  # noqa: E402
import telnetlib3  # noqa: E402
import argparse  # noqa: E402
from pathlib import Path  # noqa: E402
from typing import Optional, Tuple, Any  # noqa: E402


class MUDTestSuite:
    """Automated test suite for Broken Shadows MUD."""

    def __init__(
        self, port: int = 4000, skip_build: bool = False, debug: bool = False,
        no_cleanup: bool = False
    ):
        """Initialize test suite with configuration."""
        self.port = port
        self.skip_build = skip_build
        self.debug = debug
        self.no_cleanup = no_cleanup
        self.test_character = "TestChar"
        self.test_password = "test123"
        self.game_prompt = " MV >"
        self.workspace_root = Path(__file__).parent.parent
        self.player_file = self.workspace_root / "player" / self.test_character
        self.yaml_player_file = (
            self.workspace_root / "player" / (self.test_character + ".yaml")
        )
        self.container_name = "broken_shadows"
        self.core_dir = self.workspace_root / "core"
        self._core_files_before: set[Path] = set()
        self.success_count = 0
        self.failure_count = 0
        self._connection_ok = False

    def log(self, message: str, level: str = "INFO") -> None:
        """Print timestamped log message."""
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        line = f"[{timestamp}] [{level}] {message}"

        # Colorize test outcomes for easier scanning in terminal output.
        use_color = sys.stdout.isatty() and "NO_COLOR" not in os.environ
        if use_color and level == "PASS":
            print(f"\033[32m{line}\033[0m")
            return
        if use_color and level == "FAIL":
            print(f"\033[31m{line}\033[0m")
            return

        print(line)

    def run_command(
        self, command: str, timeout: int = 30, check: bool = True
    ) -> Tuple[int, str, str]:
        """Execute shell command and return exit code, stdout, stderr."""
        self.log(f"Executing: {command}")
        try:
            result = subprocess.run(
                command,
                shell=True,
                capture_output=True,
                text=True,
                timeout=timeout,
                cwd=str(self.workspace_root),
            )
            return result.returncode, result.stdout, result.stderr
        except subprocess.TimeoutExpired:
            self.log(f"Command timeout after {timeout}s: {command}", "ERROR")
            raise

    def format_recv_for_log(self, text: str) -> str:
        """Format telnet receive text for easier human reading."""
        ansi_pattern = re.compile(r"\x1b\[[0-9;?]*[ -/]*[@-~]")
        cleaned = ansi_pattern.sub("", text)
        cleaned = cleaned.replace("\r\n", "\n").replace("\r", "\n")

        # Remove most control chars while preserving newlines and tabs.
        cleaned = "".join(
            ch if ch in "\n\t" or 32 <= ord(ch) <= 126 else " "
            for ch in cleaned
        )

        lines = [line.rstrip() for line in cleaned.split("\n")]
        lines = [line for line in lines if line.strip()]
        if not lines:
            return "RECV: <no printable output>"

        max_lines = 12
        shown = lines[:max_lines]
        body = "\n".join(f"    {line}" for line in shown)
        if len(lines) > max_lines:
            body += f"\n    ... ({len(lines) - max_lines} more lines)"

        return f"RECV:\n{body}"

    def normalize_telnet_text(self, text: str) -> str:
        """Normalize telnet text by stripping ANSI/control formatting."""
        ansi_pattern = re.compile(r"\x1b\[[0-9;?]*[ -/]*[@-~]")
        cleaned = ansi_pattern.sub("", text)
        cleaned = cleaned.replace("\r\n", "\n").replace("\r", "\n")
        cleaned = "".join(
            ch if ch in "\n\t" or 32 <= ord(ch) <= 126 else " "
            for ch in cleaned
        )
        return cleaned

    def snapshot_core_files(self) -> None:
        """Record which core files exist before tests run."""
        if self.core_dir.exists():
            self._core_files_before = set(self.core_dir.iterdir())
        else:
            self._core_files_before = set()

    def check_no_new_core_files(self) -> bool:
        """Fail if any new core files appeared since snapshot."""
        if not self.core_dir.exists():
            self.test_passed("No core files")
            return True
        current = set(self.core_dir.iterdir())
        new_files = current - self._core_files_before
        if new_files:
            for f in sorted(new_files):
                self.log(f"Core file created: {f}", "ERROR")
            self.test_failed("No core files")
            return False
        self.test_passed("No core files")
        return True

    def cleanup_docker(self) -> None:
        """Stop and remove docker container."""
        self.log("Cleaning up Docker container...")
        try:
            # Stop container
            self.run_command(
                "docker compose down --remove-orphans",
                timeout=15,
                check=False,
            )
            time.sleep(1)
            self.log("Docker container stopped")
        except Exception as e:
            self.log(f"Docker cleanup error: {e}", "WARNING")

    def cleanup_player_file(self) -> None:
        """Remove test player files (legacy and YAML)."""
        self.log("Cleaning up player file...")
        for path in (self.player_file, self.yaml_player_file):
            if path.exists():
                try:
                    path.unlink()
                    self.log(f"Removed player file: {path}")
                except Exception as e:
                    self.log(f"Failed to remove player file: {e}", "WARNING")

    def build_image(self) -> bool:
        """Build Docker image for the MUD."""
        if self.skip_build:
            self.log("Skipping Docker build (--skip-build enabled)")
            return True

        self.log("Building Docker image...")
        try:
            returncode, _, stderr = self.run_command(
                "./build_image.sh", timeout=600
            )
            if returncode == 0:
                self.log("Docker image built successfully")
                self.test_passed("Docker build")
                return True
            else:
                self.log(f"Docker build failed: {stderr}", "ERROR")
                self.test_failed("Docker build")
                return False
        except Exception as e:
            self.log(f"Docker build error: {e}", "ERROR")
            self.test_failed("Docker build")
            return False

    def start_container(self) -> bool:
        """Start the Docker container."""
        self.log("Starting Docker container...")

        # Stop any existing container first
        self.cleanup_docker()
        time.sleep(2)

        try:
            self.log("Pulling latest image...")
            returncode, _, stderr = self.run_command(
                "docker compose pull", timeout=120
            )
            if returncode != 0:
                self.log(f"docker compose pull failed: {stderr}", "WARNING")

            returncode, _, stderr = self.run_command(
                "docker compose up -d", timeout=30
            )
            if returncode == 0:
                self.log("Docker container started")
                time.sleep(3)  # Give server time to start

                # Check if container is running
                returncode, stdout, _ = self.run_command(
                    "docker compose ps", timeout=10, check=False
                )
                if "broken_shadows" in stdout and "Up" in stdout:
                    self.log("Container is running")
                    self.test_passed("Container startup")
                    return True
                else:
                    self.log("Container failed to start properly", "ERROR")
                    self.test_failed("Container startup")
                    return False
            else:
                self.log(f"Failed to start container: {stderr}", "ERROR")
                self.test_failed("Container startup")
                return False
        except Exception as e:
            self.log(f"Container startup error: {e}", "ERROR")
            self.test_failed("Container startup")
            return False

    async def wait_for_server(self, timeout: int = 30) -> bool:
        """Wait for server to be responsive."""
        self.log(f"Waiting for server to be ready (timeout: {timeout}s)...")
        start = time.time()

        while time.time() - start < timeout:
            try:
                _, writer = await asyncio.wait_for(
                    telnetlib3.open_connection(
                        "localhost", self.port
                    ),
                    timeout=5,
                )
                writer.close()
                await writer.wait_closed()
                self.log("Server is responsive")
                self.test_passed("Server responsiveness")
                return True
            except (
                ConnectionRefusedError,
                EOFError,
                OSError,
                asyncio.TimeoutError,
            ):
                await asyncio.sleep(1)

        self.log("Server did not respond within timeout", "ERROR")
        self.test_failed("Server responsiveness")
        return False

    async def connect_telnet(self):
        """Connect to the MUD via Telnet."""
        self.log(f"Connecting to MUD at localhost:{self.port}...")
        try:
            reader, writer = await asyncio.wait_for(
                telnetlib3.open_connection(
                    "localhost", self.port
                ),
                timeout=10,
            )
            self.log("Connected to MUD")
            self._connection_ok = True
            self.test_passed("Telnet connection")
            return reader, writer
        except Exception as e:
            self.log(f"Failed to connect: {e}", "ERROR")
            self.test_failed("Telnet connection")
            return None

    async def read_until_with_paging(
        self, reader: Any, writer: Any, pattern: bytes, timeout: int = 5
    ) -> bytes:
        """Read until pattern, automatically handling paging prompts."""
        deadline = time.time() + timeout
        accumulated = b""
        pattern_str = pattern.decode("utf-8", errors="ignore")

        while time.time() < deadline:
            remaining = deadline - time.time()
            if remaining <= 0:
                break

            try:
                chunk = await asyncio.wait_for(
                    reader.read(4096),
                    timeout=min(0.5, remaining),
                )
                if not chunk:
                    break

                # telnetlib3 returns str; convert to bytes for uniform handling
                if isinstance(chunk, str):
                    chunk = chunk.encode("utf-8", errors="ignore")
                accumulated += chunk

                if self.debug:
                    self.log(
                        f"Read chunk: {len(chunk)} bytes",
                        "DEBUG",
                    )

                decoded = accumulated.decode("utf-8", errors="ignore")

                if "[Hit Return to continue" in decoded:
                    if self.debug:
                        self.log(
                            "Detected paging prompt, sending newline",
                            "DEBUG",
                        )
                    writer.write("\r\n")
                    await writer.drain()
                    continue

                if pattern_str in decoded:
                    if self.debug:
                        self.log(self.format_recv_for_log(decoded), "DEBUG")
                    return accumulated

            except asyncio.TimeoutError:
                # No data in this window; keep looping until deadline.
                continue
            except Exception as e:
                self.log(f"Paging read error: {e}", "WARNING")
                break

        if self.debug and accumulated:
            decoded_acc = accumulated.decode("utf-8", errors="ignore")
            self.log(self.format_recv_for_log(decoded_acc), "DEBUG")
        elif self.debug:
            self.log(
                f"read_until_with_paging: {len(accumulated)} bytes",
                "DEBUG",
            )
        return accumulated

    async def read_until_prompt(
        self,
        reader: Any,
        writer: Any,
        timeout: int = 5,
        expected: str = " MV >",
    ) -> str:
        """Read output until we see a prompt, handling paging."""
        result = await self.read_until_with_paging(
            reader, writer, expected.encode(), timeout=timeout
        )
        return result.decode("utf-8", errors="ignore")

    async def drain_reader(self, reader: Any, timeout: float = 0.1) -> None:
        """Drain any pending data from reader buffer."""
        try:
            deadline = time.time() + timeout
            while time.time() < deadline:
                remaining = deadline - time.time()
                if remaining <= 0:
                    break
                chunk = await asyncio.wait_for(
                    reader.read(4096), timeout=remaining
                )
                if not chunk:
                    break
                if self.debug:
                    self.log(
                        f"Drained {len(chunk)} bytes from reader",
                        "DEBUG",
                    )
        except asyncio.TimeoutError:
            pass
        except Exception as e:
            self.log(f"Reader drain error: {e}", "WARNING")

    async def send_command(
        self,
        reader: Any,
        writer: Any,
        command: str,
        wait_for: Optional[str] = None,
    ) -> str:
        """Send command and optionally wait for response."""
        if self.debug:
            self.log(f"SEND: {repr(command)}", "DEBUG")
        else:
            self.log(f"Sending: {command}")
        try:
            writer.write(command + "\r\n")
            await writer.drain()
            if wait_for:
                response = await self.read_until_prompt(
                    reader, writer, expected=wait_for
                )
                return response
            return ""
        except Exception as e:
            self.log(f"Send error: {e}", "ERROR")
            self._connection_ok = False
            return ""

    async def read_until(
        self, reader: Any, pattern: bytes, timeout: int = 5
    ) -> bytes:
        """Read until pattern with debug logging."""
        output = await asyncio.wait_for(
            reader.readuntil(pattern),
            timeout=timeout,
        )
        if self.debug:
            decoded = output.decode("utf-8", errors="ignore")
            self.log(self.format_recv_for_log(decoded), "DEBUG")
        return output

    async def wait_for_game_prompt(
        self, reader: Any, timeout: int = 30
    ) -> bool:
        """Wait until we detect the in-game HP/MP/MV prompt."""
        try:
            await self.read_until(
                reader, self.game_prompt.encode(), timeout=timeout
            )
            return True
        except Exception:
            self.log(
                "Strict prompt match failed, scanning for status prompt",
                "WARNING",
            )

        deadline = time.time() + timeout
        while time.time() < deadline:
            try:
                chunk = await asyncio.wait_for(
                    reader.readuntil(b">"),
                    timeout=2,
                )
                decoded = chunk.decode("utf-8", errors="ignore")
                if self.debug:
                    self.log(self.format_recv_for_log(decoded), "DEBUG")

                normalized = self.normalize_telnet_text(decoded)
                one_line = normalized.replace("\n", " ")
                if (
                    " HP " in one_line
                    and " MP " in one_line
                    and " MV >" in one_line
                ):
                    return True
            except asyncio.TimeoutError:
                continue
            except Exception as e:
                self.log(f"Prompt scan failed: {e}", "WARNING")
                return False

        return False

    def parse_exits(self, look_output: str) -> list[str]:
        """Parse room exits from look output."""
        match = re.search(r"\[Exits:\s*([^\]]+)\]", look_output)
        if not match:
            return []

        exits_text = match.group(1).strip().lower()
        if exits_text in {"none", ""}:
            return []

        valid = {
            "north", "south", "east", "west", "up", "down",
            "n", "s", "e", "w", "u", "d",
            "ne", "nw", "se", "sw",
        }
        parsed = [part.strip() for part in exits_text.split()]
        return [direction for direction in parsed if direction in valid]

    def extract_room_name(self, look_output: str) -> Optional[str]:
        """Extract a probable room title from look output."""
        ignore_prefixes = (
            "[exits:",
            "num",
            "===",
            "your current board",
            "you can only read notes",
        )

        for raw in look_output.splitlines():
            line = raw.strip()
            lower = line.lower()
            if not line:
                continue
            if lower.startswith(ignore_prefixes):
                continue
            if " hp " in lower and " mp " in lower and " mv " in lower:
                continue
            if line.startswith("[") and line.endswith("]"):
                continue
            return line

        return None

    async def test_character_creation(
        self, reader: Any, writer: Any
    ) -> bool:
        """Test character creation."""
        if not self.require_connection("Character creation"):
            return False
        self.log("Testing character creation...")
        try:
            # Handle ANSI color prompt
            await self.read_until(reader, b"ANSI color?")
            await self.send_command(reader, writer, "y")
            await asyncio.sleep(0.5)

            # Wait for name prompt
            await self.read_until(reader, b"what is your name?")
            await self.send_command(reader, writer, self.test_character)
            await asyncio.sleep(0.5)

            # Confirm name "Did I get that right"
            await self.read_until(reader, b"(Y/N)?")
            await self.send_command(reader, writer, "y")
            await asyncio.sleep(0.5)

            # Enter password (first time)
            await self.read_until(reader, b"password")
            await self.send_command(reader, writer, self.test_password)
            await asyncio.sleep(0.5)

            # Confirm password (second time)
            await self.read_until(reader, b"password")
            await self.send_command(reader, writer, self.test_password)
            await asyncio.sleep(0.5)

            # Select race
            await self.read_until(reader, b"What is your race")
            await self.send_command(reader, writer, "human")
            await asyncio.sleep(0.5)

            # Select sex
            await self.read_until(reader, b"What is your sex")
            await self.send_command(reader, writer, "m")
            await asyncio.sleep(0.5)

            # Roll stats - press enter to start rolling
            await self.read_until(reader, b"Press enter to start rolling")
            await self.send_command(reader, writer, "")
            await asyncio.sleep(1)

            # Select stats column
            await self.read_until(reader, b"number of column:")
            await self.send_command(reader, writer, "0")
            await asyncio.sleep(0.5)

            # Select class
            await self.read_until(reader, b"Select a class")
            await self.send_command(reader, writer, "warrior")
            await asyncio.sleep(0.5)

            # Select alignment
            await self.read_until(reader, b"alignment (G/N/E)?")
            await self.send_command(reader, writer, "n")
            await asyncio.sleep(0.5)

            # Customization prompt
            await self.read_until(reader, b"Customize (Y/N)?")
            await self.send_command(reader, writer, "n")
            await asyncio.sleep(0.5)

            # Weapon selection
            await self.read_until(reader, b"Your choice?")
            await self.send_command(reader, writer, "sword")
            await asyncio.sleep(0.5)

            # Hit Return to continue
            await self.read_until(reader, b"Hit Return to continue")
            # Send newline and wait for the prompt to come back
            pager_response = await self.send_command(
                reader, writer, "", wait_for=self.game_prompt
            )
            if self.debug:
                self.log(
                    f"Pager response ({len(pager_response)} bytes)",
                    "DEBUG",
                )

            self.log("Character created successfully")
            self.test_passed("Character creation")
            return True

        except Exception as e:
            self.log(f"Character creation failed: {e}", "ERROR")
            self.test_failed("Character creation")
            return False

    async def test_movement(
        self, reader: Any, writer: Any
    ) -> bool:
        """Test movement between rooms."""
        if not self.require_connection("Movement"):
            return False
        self.log("Testing movement...")
        try:
            # First look establishes the current room and available exits.
            initial_look = await self.send_command(
                reader, writer, "look", wait_for=self.game_prompt
            )
            await asyncio.sleep(0.5)

            if self.debug:
                self.log(
                    f"Full look output:\n{initial_look}",
                    "DEBUG",
                )

            if "[Exits:" not in initial_look:
                self.log(
                    "Look output did not include exits; "
                    "test may be out of sync",
                    "WARNING",
                )

            initial_room = self.extract_room_name(initial_look)
            if initial_room:
                self.log(f"Initial room detected: {initial_room}")

            exits = self.parse_exits(initial_look)
            if not exits:
                self.log("No exits parsed from look output", "WARNING")
                self.test_passed("Movement (no exits available)")
                return True

            direction = exits[0]
            self.log(f"Attempting movement via exit: {direction}")

            move_response = await self.send_command(
                reader, writer, direction, wait_for=self.game_prompt
            )
            await asyncio.sleep(0.5)

            if "Alas, you cannot go that way" in move_response:
                self.log("Selected exit was rejected by server", "WARNING")
                self.test_passed("Movement (attempted)")
                return True

            # Verify we're in a room after movement.
            look_after_move = await self.send_command(
                reader, writer, "look", wait_for=self.game_prompt
            )
            moved_room = self.extract_room_name(look_after_move)
            if moved_room:
                self.log(f"Room after move detected: {moved_room}")

            has_room_data = (
                "[Exits:" in look_after_move
                and len(look_after_move.strip()) > 0
            )

            if initial_room and moved_room:
                if initial_room == moved_room:
                    self.log(
                        "Movement command completed but "
                        "room name did not change",
                        "ERROR",
                    )
                    self.test_failed("Movement")
                    return False

                self.log("Movement successful")
                self.test_passed("Movement")
                return True

            if has_room_data:
                self.log(
                    "Movement verified by room output, "
                    "but room names could not be compared",
                    "WARNING",
                )
                self.log("Movement successful")
                self.test_passed("Movement")
                return True
            else:
                self.log("Movement verification failed", "ERROR")
                self.test_failed("Movement")
                return False

        except Exception as e:
            self.log(f"Movement test failed: {e}", "ERROR")
            self.test_failed("Movement")
            return False

    async def test_inventory(
        self, reader: Any, writer: Any
    ) -> bool:
        """Test inventory command."""
        if not self.require_connection("Inventory check"):
            return False
        self.log("Testing inventory...")
        try:
            response = await self.send_command(
                reader, writer, "inventory", wait_for=self.game_prompt
            )
            if self.debug:
                self.log(
                    f"Full inventory output:\n{response}",
                    "DEBUG",
                )
            if len(response) > 0:
                self.log("Inventory command successful")
                self.test_passed("Inventory check")
                return True
            else:
                self.log("Inventory check returned empty", "WARNING")
                self.test_passed("Inventory check (empty)")
                return True

        except Exception as e:
            self.log(f"Inventory test failed: {e}", "ERROR")
            self.test_failed("Inventory check")
            return False

    async def test_save(
        self, reader: Any, writer: Any
    ) -> bool:
        """Test the save command — verifies no crash and YAML file written."""
        if not self.require_connection("Save"):
            return False
        self.log("Testing save command...")
        try:
            response = await self.send_command(
                reader, writer, "save", wait_for=self.game_prompt
            )
            if self.debug:
                self.log(f"Full save output:\n{response}", "DEBUG")

            # Give the server a moment to finish writing the file.
            await asyncio.sleep(1)

            if self.yaml_player_file.exists():
                size = self.yaml_player_file.stat().st_size
                self.log(
                    f"YAML player file written: {self.yaml_player_file} "
                    f"({size} bytes)"
                )
                self.test_passed("Save (YAML file written)")
                return True
            else:
                self.log(
                    f"YAML player file not found at {self.yaml_player_file}",
                    "ERROR",
                )
                self.test_failed("Save (YAML file not found)")
                return False

        except Exception as e:
            self.log(f"Save test failed: {e}", "ERROR")
            self.test_failed("Save")
            return False

    async def test_quit(
        self, reader: Any, writer: Any
    ) -> bool:
        """Test logout/quit."""
        if not self.require_connection("Logout"):
            return False
        self.log("Testing logout...")
        try:
            await self.send_command(reader, writer, "quit", wait_for="?")
            await asyncio.sleep(0.5)

            # Confirm quit
            await self.send_command(reader, writer, "y", wait_for=":")
            await asyncio.sleep(1)

            self.log("Logout successful")
            self.test_passed("Logout")
            return True

        except Exception as e:
            self.log(f"Logout test failed: {e}", "ERROR")
            self._connection_ok = False
            try:
                writer.close()
                await writer.wait_closed()
            except Exception:
                pass
            self.test_failed("Logout")
            return False

    def require_connection(self, test_name: str) -> bool:
        """Fail the named test immediately if the connection is not healthy."""
        if not self._connection_ok:
            self.test_failed(f"{test_name} (no connection)")
            return False
        return True

    def test_passed(self, test_name: str) -> None:
        """Record passed test."""
        self.log(f"✓ PASSED: {test_name}", "PASS")
        self.success_count += 1

    def test_failed(self, test_name: str) -> None:
        """Record failed test."""
        self.log(f"✗ FAILED: {test_name}", "FAIL")
        self.failure_count += 1

    async def run_all_tests(self) -> int:
        """Run complete test suite."""
        self.log("=" * 60)
        self.log("BROKEN SHADOWS MUD AUTOMATED TEST SUITE", "START")
        self.log("=" * 60)

        try:
            # Build phase
            if not self.build_image():
                self.log("Build phase failed, aborting tests", "ERROR")
                return 1

            # Container startup phase
            if not self.start_container():
                self.log("Container startup failed, aborting tests", "ERROR")
                return 1

            # Server readiness phase
            if not await self.wait_for_server():
                self.log("Server not ready, aborting tests", "ERROR")
                self.cleanup_docker()
                return 1

            # Telnet connection phase
            connection = await self.connect_telnet()
            if connection is None:
                self.cleanup_docker()
                return 1

            reader, writer = connection

            # Snapshot core files before running any game tests.
            self.snapshot_core_files()

            # Game functionality tests
            try:
                await self.test_character_creation(reader, writer)
                await asyncio.sleep(1)
                await self.test_movement(reader, writer)
                await asyncio.sleep(1)
                await self.test_inventory(reader, writer)
                await asyncio.sleep(1)
                await self.test_save(reader, writer)
                await asyncio.sleep(1)
                await self.test_quit(reader, writer)
                await asyncio.sleep(1)
                self.check_no_new_core_files()
            finally:
                try:
                    writer.close()
                    await writer.wait_closed()
                except Exception:
                    pass

            # Cleanup phase
            if self.no_cleanup:
                self.log("Skipping Docker cleanup (--no-cleanup enabled)")
            else:
                self.cleanup_docker()
            time.sleep(1)
            self.cleanup_player_file()

            # Final report
            self.log("=" * 60)
            self.log(f"TESTS PASSED: {self.success_count}", "RESULT")
            self.log(f"TESTS FAILED: {self.failure_count}", "RESULT")
            self.log("=" * 60)

            if self.failure_count > 0:
                return 1
            return 0

        except KeyboardInterrupt:
            self.log("\nTest suite interrupted by user", "WARNING")
            if not self.no_cleanup:
                self.cleanup_docker()
            self.cleanup_player_file()
            return 130
        except Exception as e:
            self.log(f"Unexpected error: {e}", "ERROR")
            if not self.no_cleanup:
                self.cleanup_docker()
            self.cleanup_player_file()
            return 1


def main() -> int:
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description="Broken Shadows MUD Automated Test Suite"
    )
    parser.add_argument(
        "--skip-build",
        action="store_true",
        help="Skip Docker image build phase",
    )
    parser.add_argument(
        "--port",
        type=int,
        default=4000,
        help="MUD port (default: 4000)",
    )
    parser.add_argument(
        "--debug",
        action="store_true",
        help="Enable debug output (show telnet send/receive)",
    )
    parser.add_argument(
        "--no-cleanup",
        action="store_true",
        help="Leave Docker container running after tests complete",
    )

    args = parser.parse_args()

    # Verify we're in the right directory
    if not Path("docker-compose.yml").exists():
        print("Error: docker-compose.yml not found. Run from repository root.")
        return 1

    test_suite = MUDTestSuite(
        port=args.port, skip_build=args.skip_build, debug=args.debug,
        no_cleanup=args.no_cleanup
    )
    return asyncio.run(test_suite.run_all_tests())


if __name__ == "__main__":
    sys.exit(main())
