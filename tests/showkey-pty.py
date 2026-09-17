"""Run showkey's input loops with real PTYs and simulated device events."""
import fcntl
import os
import pty
import select
import signal
import struct
import subprocess
import sys
import termios
import time


def controlling_tty():
    os.setsid()
    fcntl.ioctl(0, termios.TIOCSCTTY, 0)


def collect(process, master, output, until_prompt=False):
    deadline = time.monotonic() + 4

    while time.monotonic() < deadline:
        if select.select([master], [], [], 0.05)[0]:
            output += os.read(master, 4096)

        if until_prompt and b"last keypress)" in output:
            return output

        if process.poll() is not None:
            while select.select([master], [], [], 0)[0]:
                output += os.read(master, 4096)

            if until_prompt:
                raise AssertionError(("exited before prompt", output))

            return output

    raise AssertionError(("showkey did not finish", output))


def run_case(helper, backend, payload=b"", external_signal=None):
    master, slave = pty.openpty()
    source_master, source_slave = pty.openpty()
    pipe_read, pipe_write = os.pipe()
    descriptors = (master, slave, source_master, source_slave, pipe_read, pipe_write)
    original = termios.tcgetattr(slave)
    source_original = termios.tcgetattr(source_slave)
    process = None

    try:
        if backend == "evdev":
            args = ["evdev", str(pipe_read)]

        elif backend == "identity-failure":
            args = [backend]

        elif backend in ("cleanup-failure", "setup-failure"):
            args = [backend, os.ttyname(source_slave)]

        else:
            mode = "scancodes" if backend.endswith("scan") else "console"
            path = os.ttyname(slave if backend.startswith("same") else source_slave)
            args = [mode, path]

        process = subprocess.Popen(
            [helper] + args, stdin=slave, stdout=slave, stderr=slave,
            pass_fds=(pipe_read,), preexec_fn=controlling_tty,
        )

        immediate = backend in ("identity-failure", "cleanup-failure", "setup-failure")

        if immediate:
            output = collect(process, master, b"")

        else:
            output = collect(process, master, b"", until_prompt=True)
            current = termios.tcgetattr(slave)

            assert not current[3] & (termios.ICANON | termios.ECHO | termios.ISIG)
            assert not current[0] & termios.IXON

            os.write(master, payload)

            # Produce output after Ctrl-S has had a chance to stop the tty.
            time.sleep(0.1)

            if backend == "evdev":
                os.write(pipe_write, struct.pack("@llHHi", 0, 0, 1, 30, 1))
            else:
                os.write(master if backend.startswith("same") else source_master, b"\x1e")

            if external_signal is not None:
                os.kill(process.pid, external_signal)

            output = collect(process, master, output)

        expected = 128 + external_signal if external_signal else 0

        if backend == "setup-failure":
            expected = 1

        assert process.returncode == expected, (process.returncode, output)
        assert termios.tcgetattr(slave) == original, "controlling tty not restored"
        assert termios.tcgetattr(source_slave) == source_original, "selected tty not restored"

        if not immediate and not external_signal:
            assert (b"0x1e" if backend.endswith("scan") else b"keycode  30 press") in output, output

        for devfd in (slave, source_slave):
            os.set_blocking(devfd, False)

            try:
                remaining = os.read(devfd, 1024)
            except BlockingIOError:
                remaining = b""

            assert not remaining, ("input left for shell", remaining)

        print(backend, repr(payload), external_signal, "ok")

    finally:
        if process is not None and process.poll() is None:
            process.kill()
            process.wait()

        for devfd in descriptors:
            os.close(devfd)


helper = os.path.abspath(sys.argv[1])

for payload in (b"shell-input\n", b"\x03", b"\x13", b"\x1a"):
    run_case(helper, "evdev", payload)

run_case(helper, "evdev", b"pending", signal.SIGTERM)

for backend in ("same", "same-scan", "different", "different-scan"):
    run_case(helper, backend, b"\x03\x13\x1a" if backend.startswith("same") else b"shell-input\n\x03\x13")

run_case(helper, "different", b"pending", signal.SIGTERM)
run_case(helper, "identity-failure")
run_case(helper, "cleanup-failure")
run_case(helper, "setup-failure")
