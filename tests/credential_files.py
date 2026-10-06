"""Exercise SOCKS authentication with disposable credential files."""

import os
import socket
import subprocess
import sys
import tempfile
import time
from pathlib import Path

BINARY = str(Path(sys.argv[1]).resolve())
USER = b"fixture-reader"
PASSWORD = b"fixture-password"


def _free_port() -> int:
    with socket.socket() as sock:
        sock.bind(("127.0.0.1", 0))
        return sock.getsockname()[1]


def _connect(port: int, process: subprocess.Popen) -> socket.socket:
    for _ in range(100):
        if process.poll() is not None:
            raise AssertionError("Proxy exited before accepting authentication")
        try:
            return socket.create_connection(("127.0.0.1", port), timeout=1)
        except ConnectionRefusedError:
            time.sleep(0.01)
    raise AssertionError("Proxy did not start")


def _recv_exact(sock: socket.socket, count: int) -> bytes:
    data = bytearray()
    while len(data) < count:
        chunk = sock.recv(count - len(data))
        if not chunk:
            raise AssertionError("Proxy closed during authentication")
        data.extend(chunk)
    return bytes(data)


def _authentication(port: int, process: subprocess.Popen, password: bytes) -> bytes:
    with _connect(port, process) as sock:
        sock.sendall(b"\x05\x01\x02")
        assert _recv_exact(sock, 2) == b"\x05\x02"
        sock.sendall(
            b"\x01" + bytes([len(USER)]) + USER + bytes([len(password)]) + password
        )
        return _recv_exact(sock, 2)


with tempfile.TemporaryDirectory() as directory:
    user = Path(directory) / "username"
    password = Path(directory) / "password"
    user.write_bytes(USER + b"\n")
    password.write_bytes(PASSWORD + b"\r\n")
    env = {
        **os.environ,
        "MICROSOCKS_USERNAME_FILE": str(user),
        "MICROSOCKS_PASSWORD_FILE": str(password),
    }
    port = _free_port()
    args = [BINARY, "-i", "127.0.0.1", "-p", str(port), "-q"]
    process = subprocess.Popen(
        args, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    try:
        assert _authentication(port, process, PASSWORD) == b"\x01\x00"
        assert _authentication(port, process, b"wrong") == b"\x01\x02"
        with _connect(port, process) as sock:
            sock.sendall(b"\x05\x01\x00")
            assert _recv_exact(sock, 2) == b"\x05\xff"
        proc = Path(f"/proc/{process.pid}")
        if proc.exists():
            assert USER not in (proc / "cmdline").read_bytes()
            assert PASSWORD not in (proc / "cmdline").read_bytes()
            assert USER not in (proc / "environ").read_bytes()
            assert PASSWORD not in (proc / "environ").read_bytes()
    finally:
        process.terminate()
        output, errors = process.communicate(timeout=5)
        assert USER not in output + errors
        assert PASSWORD not in output + errors
    password.write_bytes(b"x" * 255 + b"\r\n")
    port = _free_port()
    args = [BINARY, "-i", "127.0.0.1", "-p", str(port), "-q"]
    process = subprocess.Popen(
        args, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    try:
        assert _authentication(port, process, b"x" * 255) == b"\x01\x00"
    finally:
        process.terminate()
        process.communicate(timeout=5)
    for invalid in (b"", b"x" * 256, b"x\ny", b"x\0y", b"x\r"):
        password.write_bytes(invalid)
        result = subprocess.run(
            args, env=env, capture_output=True, timeout=5, check=False
        )
        assert result.returncode != 0
    password.write_bytes(PASSWORD)
    for bad_env, flags in (
        (env, ["-P", "fixture-password"]),
        (env, ["-u", "fixture-reader"]),
    ):
        result = subprocess.run(
            args + flags, env=bad_env, capture_output=True, timeout=5, check=False
        )
        assert result.returncode != 0
    password.write_bytes(PASSWORD)
    for bad_env, flags in (
        ({**env, "MICROSOCKS_PASSWORD_FILE": str(password) + ".missing"}, []),
        (
            {
                key: value
                for key, value in env.items()
                if key != "MICROSOCKS_PASSWORD_FILE"
            },
            [],
        ),
        (env, ["-u", "fixture-reader", "-P", "fixture-password"]),
    ):
        result = subprocess.run(
            args + flags, env=bad_env, capture_output=True, timeout=5, check=False
        )
        assert result.returncode != 0
    legacy_env = {
        key: value
        for key, value in os.environ.items()
        if key not in ("MICROSOCKS_USERNAME_FILE", "MICROSOCKS_PASSWORD_FILE")
    }
    legacy = subprocess.Popen(
        args + ["-u", USER.decode(), "-P", PASSWORD.decode()],
        env=legacy_env,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    try:
        assert _authentication(port, legacy, PASSWORD) == b"\x01\x00"
    finally:
        legacy.terminate()
        legacy.communicate(timeout=5)
print("MicroSocks file credentials: authentication and fail-closed checks passed")
