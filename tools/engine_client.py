import os
import subprocess
from pathlib import Path
from typing import Iterable, Optional


class EngineClientError(RuntimeError):
    pass


class EngineClient:
    def __init__(self, executable: str) -> None:
        self.executable = str(executable)
        self.process: Optional[subprocess.Popen[str]] = None
        self.log: list[str] = []

    @classmethod
    def find_default(cls) -> "EngineClient":
        repo_root = Path(__file__).resolve().parent.parent
        candidates = (
            repo_root / "src" / "uci.exe",
            repo_root / "src" / "uci",
        )
        executable = next((str(path) for path in candidates if path.is_file()), "")
        return cls(executable)

    def start(self) -> None:
        if not self.executable:
            raise EngineClientError(
                "Engine executable was not found. Build src/uci.exe or src/uci first."
            )
        if not os.path.isfile(self.executable):
            raise EngineClientError("Engine executable does not exist: " + self.executable)
        if self.process and self.process.poll() is None:
            return

        try:
            executable = str(Path(self.executable).resolve())
            popen_kwargs = {
                "stdin": subprocess.PIPE,
                "stdout": subprocess.PIPE,
                "stderr": subprocess.PIPE,
                "text": True,
                "bufsize": 1,
                "cwd": str(Path(executable).parent),
                "close_fds": True,
            }
            self.process = subprocess.Popen(
                [executable],
                **popen_kwargs,
            )
        except OSError as exc:
            raise EngineClientError("Could not start engine: " + str(exc)) from exc

        self._send("uci")
        if not self._read_until("uciok", timeout_lines=20):
            # Some Windows process wrappers can buffer or lose the first startup
            # response. Verify readiness directly before reporting a failure.
            self._send("isready")
            if not self._read_until("readyok", timeout_lines=20):
                self._raise_startup_error("uciok or readyok")
        else:
            self._send("isready")
            if not self._read_until("readyok", timeout_lines=20):
                self._raise_startup_error("readyok")

    def _raise_startup_error(self, expected: str) -> None:
        exit_code = self.process.poll() if self.process else None
        stderr = ""
        if self.process and self.process.stderr:
            try:
                stderr = self.process.stderr.read().strip()
            except (OSError, ValueError):
                stderr = ""

        detail = (
            "Engine did not respond with " + expected + ". "
            "Executable: " + self.executable + ". "
        )
        if exit_code is not None:
            detail += " Process exited with code " + str(exit_code) + "."
        if stderr:
            detail += " stderr: " + stderr
        raise EngineClientError(detail)

    def close(self) -> None:
        process = self.process
        if not process:
            return

        if process.poll() is None:
            try:
                self._send("quit")
            except EngineClientError:
                pass
            try:
                process.wait(timeout=2)
            except subprocess.TimeoutExpired:
                process.terminate()

        for stream in (process.stdin, process.stdout, process.stderr):
            if stream:
                try:
                    stream.close()
                except (OSError, ValueError):
                    pass
        self.process = None

    def set_position(self, fen: str = "", moves: Iterable[str] = ()) -> None:
        command = "position "
        if fen:
            command += "fen " + fen
        else:
            command += "startpos"

        move_list = [move.strip().lower() for move in moves if move.strip()]
        if move_list:
            command += " moves " + " ".join(move_list)

        self._send(command)

    def perft(self, depth: int) -> int:
        if depth < 0:
            raise EngineClientError("Perft depth cannot be negative.")

        self._send("perft " + str(depth))
        response = self._read_response_line(timeout_lines=100)
        try:
            return int(response)
        except ValueError as exc:
            raise EngineClientError(
                "Expected an integer perft result, received: " + repr(response)
            ) from exc

    def get_fen(self) -> str:
        self._send("getfen")
        return self._read_response_line(timeout_lines=20)

    def get_turn(self) -> str:
        self._send("turn")
        response = self._read_response_line(timeout_lines=20)
        if response not in {"R", "B", "Y", "G"}:
            raise EngineClientError("Expected R, B, Y, or G; received: " + repr(response))
        return response

    def legal_moves(self) -> list[str]:
        self._send("legalmoves")
        response = self._read_response_line(timeout_lines=100)
        return response.split() if response else []

    def _send(self, command: str) -> None:
        if not self.process or not self.process.stdin:
            raise EngineClientError("Engine is not running.")
        if self.process.poll() is not None:
            raise EngineClientError("Engine process exited unexpectedly.")
        if self.process.stdin.closed:
            raise EngineClientError("Engine stdin is closed. Restart the engine.")

        try:
            self.process.stdin.write(command + "\n")
            self.process.stdin.flush()
        except (BrokenPipeError, OSError, ValueError) as exc:
            exit_code = self.process.poll()
            stderr = ""
            if self.process.stderr:
                try:
                    stderr = self.process.stderr.read().strip()
                except (OSError, ValueError):
                    pass

            detail = "Could not write to engine stdin: " + str(exc)
            if exit_code is not None:
                detail += ". Process exited with code " + str(exit_code) + "."
            if stderr:
                detail += " stderr: " + stderr
            raise EngineClientError(detail) from exc
        self.log.append("> " + command)

    def _read_response_line(self, timeout_lines: int) -> str:
        if not self.process or not self.process.stdout:
            raise EngineClientError("Engine output is unavailable.")

        for _ in range(timeout_lines):
            line = self.process.stdout.readline()
            if line == "":
                raise EngineClientError("Engine closed its output unexpectedly.")
            line = line.strip()
            if not line:
                continue
            self.log.append("< " + line)
            return line

        raise EngineClientError("Engine response exceeded the line limit.")

    def _read_until(self, expected: str, timeout_lines: int) -> bool:
        if not self.process or not self.process.stdout:
            return False

        for _ in range(timeout_lines):
            line = self.process.stdout.readline()
            if line == "":
                return False
            line = line.strip()
            if not line:
                continue
            self.log.append("< " + line)
            if line == expected:
                return True
        return False
