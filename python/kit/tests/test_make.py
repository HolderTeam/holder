"""Developer discovery/setup contracts, with build/network commands isolated."""

from __future__ import annotations

import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
from typing import Sequence

import pytest

from scripts import develop


PIN = "a" * 40


def create_core(path: Path, revision: str = PIN) -> Path:
    for name in ("CMakeLists.txt", "include/holder/holder.h", "schema/schema.sql", "README.md"):
        target = path / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text("fixture", encoding="utf-8")
    (path / ".git").mkdir(exist_ok=True)
    (path / ".revision").write_text(revision, encoding="utf-8")
    return path


@pytest.fixture
def repo(tmp_path: Path) -> Path:
    # Kit's directory in a holder checkout; tests create core/ beside python/ as needed.
    root = tmp_path / "checkout with spaces/python/kit"
    (root / "scripts").mkdir(parents=True)
    shutil.copyfile(develop.ROOT / "make.sh", root / "make.sh")
    shutil.copyfile(develop.ROOT / "scripts/develop.py", root / "scripts/develop.py")
    return root


def core_dir(repo: Path) -> Path:
    return repo.parents[1] / "core"


class FakeCommands:
    def __init__(self) -> None:
        self.calls: list[list[str]] = []
        self.envs: list[dict[str, str]] = []
        self.failure: str | None = None

    def run(self, developer: develop.Developer, args: Sequence[str], *, capture: bool = False) -> str:
        command = list(args)
        self.calls.append(command)
        self.envs.append(developer.env.copy())
        if self.failure and self.failure in command:
            raise subprocess.CalledProcessError(7, command)
        if command[0] == "git":
            source = Path(command[2])
            operation = command[3:]
            if operation == ["rev-parse", "--show-toplevel"]:
                return str(source)
            elif operation == ["rev-parse", "HEAD"]:
                return (source / ".revision").read_text()
            elif operation == ["status", "--porcelain", "--", "."]:
                return " M README.md" if (source / ".dirty").exists() else ""
        elif command == ["cmake", "--version"]:
            return "cmake version 4.3.0"
        elif command[1:3] == ["-m", "venv"]:
            python = Path(command[-1]) / "bin/python"
            python.parent.mkdir(parents=True)
            python.touch()
        elif command == ["brew", "--prefix"]:
            return str(developer.root / "homebrew")
        return ""


@pytest.fixture
def fake(monkeypatch: pytest.MonkeyPatch) -> FakeCommands:
    commands = FakeCommands()

    def run(developer: develop.Developer, args: Sequence[str], *, capture: bool = False) -> str:
        return commands.run(developer, args, capture=capture)

    monkeypatch.setattr(develop.Developer, "run", run)
    monkeypatch.setattr(shutil, "which", lambda command, path=None: f"/tools/{command}")
    return commands


def cli(repo: Path, *args: str, **env: str) -> int:
    return develop.main(args, root=repo, env=env)


@pytest.mark.parametrize("argument", ["help", "--help", "-h", "unknown"])
def test_help_invalid_command_and_no_side_effects(repo: Path, fake: FakeCommands, argument: str) -> None:
    assert cli(repo, argument) == (2 if argument == "unknown" else 0)
    assert not fake.calls
    assert not (repo / ".venv").exists()


def test_missing_core_advice_precedes_setup(repo: Path, fake: FakeCommands, capsys: pytest.CaptureFixture[str]) -> None:
    assert cli(repo) == 2
    advice = capsys.readouterr().err
    assert f"Missing {core_dir(repo)}" in advice and "HOLDER_KIT_CORE_SOURCE" in advice
    assert not fake.calls
    assert not (repo / ".venv").exists()


def test_repository_core_default_and_arguments(repo: Path, fake: FakeCommands) -> None:
    core = create_core(core_dir(repo))
    (core / ".dirty").touch()
    assert cli(repo, "test", "Debug", "-k", "connections or graph") == 0
    assert fake.calls[-1][1:] == ["-m", "pytest", "-k", "connections or graph"]
    pip = next(call for call in fake.calls if "pip" in call)
    assert f"--config-settings=cmake.define.HOLDER_KIT_CORE_SOURCE={core}" in pip
    assert "--config-settings=cmake.build-type=Debug" in pip
    assert not any("fetch" in call or "checkout" in call or "pull" in call for call in fake.calls)
    provenance = json.loads((repo / "out/make/core-source.json").read_text())
    assert provenance == {"path": str(core), "commit": PIN, "dirty": True}


def test_explicit_source_precedence_and_invalid_path(repo: Path, fake: FakeCommands) -> None:
    create_core(core_dir(repo))
    selected = create_core(repo / "custom core")
    assert cli(repo, "build", HOLDER_KIT_CORE_SOURCE="custom core", HOLDER_CORE_SDK="ignored") == 0
    assert f"--config-settings=cmake.define.HOLDER_KIT_CORE_SOURCE={selected}" in fake.calls[-1]
    assert "HOLDER_CORE_SDK" not in fake.envs[-1]
    fake.calls.clear()
    assert cli(repo, "build", HOLDER_KIT_CORE_SOURCE="missing") == 2
    assert not fake.calls


def test_invalid_repository_core_is_reported(repo: Path, fake: FakeCommands, capsys: pytest.CaptureFixture[str]) -> None:
    core_dir(repo).mkdir()
    assert cli(repo) == 2
    assert "Not a usable core source directory" in capsys.readouterr().err
    assert not fake.calls


def test_core_outside_git_has_unknown_revision(repo: Path, fake: FakeCommands) -> None:
    create_core(core_dir(repo))
    fake.failure = "--show-toplevel"
    assert cli(repo, "check") == 0
    assert fake.calls[-1][1:] == ["-m", "mypy"]
    provenance = json.loads((repo / "out/make/core-source.json").read_text())
    assert provenance["commit"] is None and provenance["dirty"] is False


def test_cmake_failure_precedes_venv_and_packages(repo: Path, fake: FakeCommands, capsys: pytest.CaptureFixture[str]) -> None:
    source = create_core(core_dir(repo))
    fake.failure = "-DBUILD_TESTING=OFF"
    assert cli(repo) == 2
    assert str(source / "README.md") in capsys.readouterr().err
    assert not (repo / ".venv").exists()
    assert not any("pip" in call for call in fake.calls)


def test_check_propagates_test_failure(repo: Path, fake: FakeCommands) -> None:
    create_core(core_dir(repo))
    fake.failure = "pytest"
    assert cli(repo, "check") == 7
    assert not any("mypy" in call for call in fake.calls)


def test_sdk_requires_a_prepared_sdk(repo: Path, fake: FakeCommands, capsys: pytest.CaptureFixture[str]) -> None:
    create_core(core_dir(repo))
    assert cli(repo, "--sdk", "test") == 2
    assert "HOLDER_CORE_SDK" in capsys.readouterr().err
    assert not fake.calls and not (repo / ".venv").exists()
    assert cli(repo, "--sdk", "test", HOLDER_CORE_SDK="missing") == 2
    assert not fake.calls


def test_prepared_sdk_and_conflicting_flags(repo: Path, fake: FakeCommands) -> None:
    sdk = repo / "sdk"
    sdk.mkdir()
    (sdk / "libholder-manifest.json").write_text("{}")
    assert cli(repo, "build", HOLDER_CORE_SDK="sdk") == 0
    assert not any("-DBUILD_TESTING=OFF" in call for call in fake.calls)
    assert "--config-settings=cmake.define.HOLDER_KIT_CORE_SOURCE=" in fake.calls[-1]
    assert fake.envs[-1]["HOLDER_CORE_SDK"] == str(sdk)
    fake.calls.clear()
    assert cli(repo, "--sdk", "wheel", "Release", HOLDER_CORE_SDK="sdk") == 0
    assert "out/make/wheels" in fake.calls[-1]
    fake.calls.clear()
    assert cli(repo, "--sdk", HOLDER_KIT_CORE_SOURCE="core") == 2
    assert not fake.calls


@pytest.mark.parametrize("command", ["setup-core", "sdk"])
def test_removed_commands(repo: Path, fake: FakeCommands, command: str) -> None:
    assert cli(repo, command) == 2
    assert not fake.calls


def test_clean_retains_venv_and_refuses_symlinks(repo: Path, fake: FakeCommands) -> None:
    for name in ("build/make", "build/other", "out/make", ".venv"):
        (repo / name).mkdir(parents=True)
    assert cli(repo, "clean") == 0
    assert not (repo / "build/make").exists() and not (repo / "out/make").exists()
    assert all((repo / name).exists() for name in ("build/other", ".venv"))
    (repo / "build/make").symlink_to(repo / "build/other", target_is_directory=True)
    assert cli(repo, "clean") == 2


@pytest.mark.parametrize("system", ["Linux", "Darwin", "FreeBSD", "OpenBSD", "NetBSD", "DragonFly"])
def test_portable_source_route(repo: Path, fake: FakeCommands, monkeypatch: pytest.MonkeyPatch, system: str) -> None:
    monkeypatch.setattr(platform, "system", lambda: system)
    create_core(core_dir(repo))
    brew = repo / "homebrew"
    (brew / "opt/openssl@3").mkdir(parents=True)
    assert cli(repo, "build") == 0
    assert not any("fetch" in call for call in fake.calls)
    if system == "Darwin":
        assert str(brew / "opt/openssl@3") in fake.envs[-1]["CMAKE_PREFIX_PATH"]


def test_launcher_from_another_directory(repo: Path) -> None:
    env = os.environ.copy()
    env["HOLDER_KIT"] = sys.executable
    result = subprocess.run(
        ["bash", str(repo / "make.sh"), "--help"], cwd=repo.parent,
        env=env, capture_output=True, text=True, check=False,
    )
    assert result.returncode == 0 and "../../core" in result.stdout
    assert not (repo / ".venv").exists()
