from __future__ import annotations

import ctypes
import hashlib
import json
import os
import platform
import shutil
import sys
import tempfile
import traceback
from collections.abc import Callable, Iterator
from contextlib import AbstractContextManager, contextmanager
from ctypes import wintypes
from pathlib import Path

import holderkit
import pytest


def _loaded_windows_modules() -> list[dict[str, str]]:
    if sys.platform != "win32":
        return []
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    enumerate_modules = kernel.K32EnumProcessModules
    enumerate_modules.argtypes = [
        wintypes.HANDLE, ctypes.POINTER(wintypes.HMODULE), wintypes.DWORD,
        ctypes.POINTER(wintypes.DWORD),
    ]
    enumerate_modules.restype = wintypes.BOOL
    kernel.GetCurrentProcess.restype = wintypes.HANDLE
    module_path = kernel.GetModuleFileNameW
    module_path.argtypes = [wintypes.HMODULE, wintypes.LPWSTR, wintypes.DWORD]
    module_path.restype = wintypes.DWORD

    needed = wintypes.DWORD()
    modules = (wintypes.HMODULE * 256)()
    while True:
        if not enumerate_modules(kernel.GetCurrentProcess(), modules, ctypes.sizeof(modules), ctypes.byref(needed)):
            raise ctypes.WinError(ctypes.get_last_error())
        if needed.value <= ctypes.sizeof(modules):
            break
        modules = (wintypes.HMODULE * (needed.value // ctypes.sizeof(wintypes.HMODULE)))()

    paths = ctypes.create_unicode_buffer(32768)
    result: list[dict[str, str]] = []
    for module in modules[:needed.value // ctypes.sizeof(wintypes.HMODULE)]:
        length = module_path(module, paths, len(paths))
        if not length or length >= len(paths):
            result.append({"handle": str(module), "error": "Could not read loaded module path"})
            continue
        path = Path(paths.value)
        entry = {"path": str(path)}
        try:
            with path.open("rb") as stream:
                digest = hashlib.sha256()
                for chunk in iter(lambda: stream.read(1024 * 1024), b""):
                    digest.update(chunk)
                entry["sha256"] = digest.hexdigest()
        except OSError as error:
            entry["error"] = str(error)
        result.append(entry)
    return result


@pytest.fixture
def sqlite_failure_capture(request: pytest.FixtureRequest) -> Callable[[Path], AbstractContextManager[None]]:
    @contextmanager
    def capture(data_dir: Path) -> Iterator[None]:
        evidence_root = os.environ.get("HOLDER_KIT_SQLITE_EVIDENCE")
        try:
            yield
        except Exception:
            if evidence_root:
                try:
                    root = Path(evidence_root)
                    root.mkdir(parents=True, exist_ok=True)
                    evidence = Path(tempfile.mkdtemp(prefix="reopened-", dir=root))
                    errors = []
                    # Copy raw files while the reopened context is still alive.
                    # Try every sidecar even if copying one file fails.
                    server = evidence / "server"
                    server.mkdir()
                    for name in ("holder.db", "holder.db-wal", "holder.db-shm"):
                        try:
                            shutil.copyfile(data_dir / "server" / name, server / name)
                        except OSError as error:
                            errors.append(f"{name}: {error}")
                    try:
                        shutil.copytree(data_dir / "projects", evidence / "projects")
                    except OSError as error:
                        errors.append(f"projects: {error}")
                    (evidence / "failure.txt").write_text(traceback.format_exc(), encoding="utf-8")
                    identity: dict[str, object] = {
                        "test": request.node.nodeid,
                        "test_order": [item.nodeid for item in request.session.items],
                        "source": os.environ.get("GITHUB_SHA"),
                        "core": os.environ.get("CORE_COMMIT"),
                        "python": sys.version,
                        "platform": platform.platform(),
                        "machine": platform.machine(),
                        "holderkit": holderkit.__file__,
                        "copy_errors": errors,
                    }
                    build = Path(holderkit.__file__).with_name("_core_build.json")
                    if build.is_file():
                        shutil.copyfile(build, evidence / build.name)
                    if sys.platform == "win32":
                        try:
                            identity["loaded_modules"] = _loaded_windows_modules()
                        except OSError as error:
                            identity["module_error"] = str(error)
                    (evidence / "identity.json").write_text(json.dumps(identity, indent=2), encoding="utf-8")
                    print(f"Captured reopened SQLite evidence in {evidence}", file=sys.stderr)
                except Exception as error:
                    print(f"SQLite failure capture failed: {error}", file=sys.stderr)
            raise

    return capture
