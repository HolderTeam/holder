#!/usr/bin/env python3
"""Run the GitHub sign-in test on emulators against the mock GitHub.

Starts the mock (unless one is already listening on the port), then for each Gradle managed
device runs GitHubMockSignInTest in a debug build pointed at the mock. Gradle creates and boots
the emulators itself; the first run downloads their system images. While the tests run, this
script keeps `adb reverse` set on every emulator, so the app reaches the mock at 127.0.0.1.
(Not at 10.0.2.2: from Android 17 an app targeting API 37 needs the local network permission
for that, while loopback is always allowed.)

    python3 mock-github/run_device_tests.py                 # Android 9 and Android 17
    python3 mock-github/run_device_tests.py pixel6Api37     # one device

Devices are defined in app/build.gradle.kts (the githubSignIn group lists the defaults).
"""

from __future__ import annotations

import argparse
import shutil
import socket
import subprocess
import sys
import threading
import time
from pathlib import Path

import mock_github

ANDROID_DIR = Path(__file__).resolve().parent.parent
DEFAULT_DEVICES = ["pixel2Api28", "pixel6Api37"]
# The Android 9 image's emulator refuses the default GPU mode for a window-less emulator.
SOFTWARE_GPU_DEVICES = {"pixel2Api28"}
SOFTWARE_GPU = "-Pandroid.testoptions.manageddevices.emulator.gpu=swiftshader_indirect"
TEST_CLASS = "team.holder.android.github.GitHubMockSignInTest"


def port_in_use(port: int) -> bool:
    with socket.socket() as probe:
        return probe.connect_ex(("127.0.0.1", port)) == 0


def start_mock(port: int) -> None:
    """Starts the mock in this process, unless one is already running."""
    if port_in_use(port):
        print(f"[run] using the mock already listening on port {port}", flush=True)
        return
    state = mock_github.MockState()
    server = mock_github.make_server(state, port)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    print(f"[run] mock GitHub on http://127.0.0.1:{port}/", flush=True)


def keep_reversing(port: int, stop: threading.Event) -> None:
    """Gradle boots each emulator during its run, so apply adb reverse as soon as one appears.
    It is idempotent, and the test waits for the mock to become reachable."""
    adb = shutil.which("adb") or "adb"
    while not stop.wait(2):
        devices = subprocess.run([adb, "devices"], capture_output=True, text=True).stdout
        for line in devices.splitlines():
            serial, _, state = line.partition("\t")
            if serial.startswith("emulator-") and state == "device":
                subprocess.run([adb, "-s", serial, "reverse", f"tcp:{port}", f"tcp:{port}"], capture_output=True)


def run_device(device: str, port: int, software_gpu: bool) -> bool:
    command = [
        str(ANDROID_DIR / "gradlew"), "-p", str(ANDROID_DIR),
        f":app:{device}DebugAndroidTest",
        "--rerun",  # the test depends on the mock and the emulator, not just its inputs
        f"-Pholder.githubMock=http://127.0.0.1:{port}",
        f"-Pandroid.testInstrumentationRunnerArguments.class={TEST_CLASS}",
        "-Pholder.android.abis=x86_64",
    ]
    if software_gpu or device in SOFTWARE_GPU_DEVICES:
        command.append(SOFTWARE_GPU)
    print(f"\n[run] {device}: {' '.join(command[3:])}", flush=True)
    started = time.monotonic()
    passed = subprocess.run(command).returncode == 0
    print(f"[run] {device}: {'passed' if passed else 'FAILED'} in {time.monotonic() - started:.0f}s", flush=True)
    return passed


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("devices", nargs="*", default=DEFAULT_DEVICES, help="managed device names")
    parser.add_argument("--port", type=int, default=mock_github.DEFAULT_PORT)
    parser.add_argument("--software-gpu", action="store_true",
                        help="render every emulator with swiftshader, as CI does (Android 9 always does)")
    args = parser.parse_args()

    start_mock(args.port)
    stop = threading.Event()
    threading.Thread(target=keep_reversing, args=(args.port, stop), daemon=True).start()
    try:
        results = {device: run_device(device, args.port, args.software_gpu) for device in args.devices}
    finally:
        stop.set()

    print("\n[run] results:")
    for device, passed in results.items():
        print(f"  {device}: {'passed' if passed else 'FAILED'}")
    if not all(results.values()):
        print(f"  reports: {ANDROID_DIR / 'app/build/reports/androidTests/managedDevice'}")
    return 0 if all(results.values()) else 1


if __name__ == "__main__":
    sys.exit(main())
