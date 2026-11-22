#!/usr/bin/env python3
"""Build a throwaway Holder sandbox full of Git conflict copies to look at in Desktop.

Everything lives under one sandbox folder: its own Holder data, config, cache and keystore,
its own daemon on its own port, and a local bare Git remote plus an "other machine" clone for
each scenario. Your real Holder data, daemon and keyring are never touched.

    python3 apps/desktop/gtk/scripts/conflict-sandbox/sandbox.py setup          # every scenario
    python3 apps/desktop/gtk/scripts/conflict-sandbox/sandbox.py setup basic crlf
    python3 apps/desktop/gtk/scripts/conflict-sandbox/sandbox.py desktop        # open Desktop on the sandbox
    python3 apps/desktop/gtk/scripts/conflict-sandbox/sandbox.py env            # exports for your own shell
    python3 apps/desktop/gtk/scripts/conflict-sandbox/sandbox.py stop | reset | list

See README.md next to this file.
"""

from __future__ import annotations

import argparse
import json
import os
import shutil
import signal
import subprocess
import sys
import tempfile
import time
from collections.abc import Callable
from dataclasses import dataclass
from datetime import date
from pathlib import Path

REPO = Path(__file__).resolve().parents[5]
EXE = ".exe" if os.name == "nt" else ""
DEFAULT_ROOT = Path(tempfile.gettempdir()) / "holder-conflict-sandbox"
DEFAULT_PORT = 11599  # The real daemon defaults to 11499; never share it.
IDLE_EXIT_SECONDS = 4 * 60 * 60  # A forgotten sandbox daemon stops itself.
GIT_IDENTITY = ["-c", "user.name=Other machine", "-c", "user.email=other@example.invalid",
                "-c", "core.autocrlf=false", "-c", "commit.gpgsign=false"]


# --- Paths and environment -------------------------------------------------------------------

def find_binary(name: str, override: str | None, env_key: str, candidates: list[Path]) -> Path:
    for value in (override, os.environ.get(env_key)):
        if value:
            path = Path(value).expanduser().resolve()
            if path.is_file():
                return path
            sys.exit(f"{name} not found at {path}")
    for path in candidates:
        if path.is_file():
            return path
    tried = "\n  ".join(str(p) for p in candidates)
    sys.exit(f"Could not find {name}. Build it, or pass --{name} / set {env_key}. Tried:\n  {tried}")


def daemon_binaries(args: argparse.Namespace) -> tuple[Path, Path]:
    build = REPO / "daemon" / "build"
    dirs = [build, build / "Debug", build / "Release", build / "RelWithDebInfo"]
    holderctl = find_binary("holderctl", args.holderctl, "HOLDER_SANDBOX_HOLDERCTL",
                            [d / f"holderctl{EXE}" for d in dirs])
    holderd = find_binary("holderd", args.holderd, "HOLDER_SANDBOX_HOLDERD",
                          [d / f"holderd{EXE}" for d in dirs])
    return holderctl, holderd


def sandbox_env(root: Path, holderctl: Path | None = None) -> dict[str, str]:
    env = {
        "XDG_DATA_HOME": str(root / "data"),
        "XDG_CONFIG_HOME": str(root / "config"),
        "XDG_STATE_HOME": str(root / "state"),
        "XDG_CACHE_HOME": str(root / "cache"),
        # Project keys go to files here instead of the system keyring.
        "HOLDER_TEST_KEYSTORE_DIR": str(root / "keys"),
    }
    if holderctl is not None:
        env["HOLDER_CTL"] = str(holderctl)
    return env


def apply_env(root: Path, holderctl: Path | None = None) -> None:
    for key, value in sandbox_env(root, holderctl).items():
        if key != "HOLDER_CTL":
            Path(value).mkdir(parents=True, exist_ok=True)
        os.environ[key] = value


def info_file(root: Path) -> Path:
    return root / "data" / "holder" / "server" / "holder.json"


def daemon_pid(root: Path) -> int | None:
    try:
        return int(json.loads(info_file(root).read_text())["pid"])
    except (OSError, ValueError, KeyError, TypeError):
        return None


def pid_alive(pid: int) -> bool:
    if os.name == "nt":
        out = subprocess.run(["tasklist", "/FI", f"PID eq {pid}", "/NH"],
                             capture_output=True, text=True, check=False).stdout
        return str(pid) in out
    try:
        os.kill(pid, 0)
    except ProcessLookupError:
        return False
    except PermissionError:
        return True
    return True


# --- Daemon ----------------------------------------------------------------------------------

def start_daemon(root: Path, holderctl: Path, holderd: Path, port: int) -> None:
    apply_env(root, holderctl)
    command = [str(holderctl), "ensure", "--json", "--daemon", str(holderd),
               "--daemon-arg", "--port", "--daemon-arg", str(port),
               "--idle-exit", str(IDLE_EXIT_SECONDS), "--workdir", str(holderd.parent)]
    result = subprocess.run(command, capture_output=True, text=True, check=False)
    try:
        report = json.loads(result.stdout)
    except ValueError:
        report = {}
    if result.returncode != 0 or not report.get("ok"):
        sys.exit(f"Sandbox daemon did not start (exit {result.returncode}).\n"
                 f"{result.stderr.strip()}\nLog: {report.get('log', root / 'cache')}")
    url = report["daemon"]["url"]
    if not url.endswith(f":{port}"):
        sys.exit(f"holderctl connected to {url}, not the sandbox port {port}. Stopping to be safe.")
    print(f"Sandbox daemon: {url} (pid {report['daemon']['pid']}, {report['state']})")


def stop_daemon(root: Path) -> None:
    pid = daemon_pid(root)
    if pid is None or not pid_alive(pid):
        print("Sandbox daemon is not running.")
        return
    if os.name == "nt":
        subprocess.run(["taskkill", "/PID", str(pid), "/T"], capture_output=True, check=False)
    else:
        os.kill(pid, signal.SIGTERM)
    for _ in range(50):
        if not pid_alive(pid):
            break
        time.sleep(0.1)
    else:
        if os.name == "nt":
            subprocess.run(["taskkill", "/PID", str(pid), "/T", "/F"], capture_output=True, check=False)
        else:
            os.kill(pid, signal.SIGKILL)
    print(f"Stopped sandbox daemon (pid {pid}).")


def connect(holderctl: Path):
    try:
        from holder import Holder
    except ImportError:
        sys.path.insert(0, str(REPO / "python" / "holder" / "src"))
        try:
            from holder import Holder
        except ImportError as error:
            sys.exit(f"Could not import the Holder Python library ({error}).\n"
                     "Install its dependencies: python3 -m pip install httpx attrs typing-extensions")
    return Holder(holderctl=str(holderctl))


# --- The other machine: plain Git on card files ----------------------------------------------

def git(*args: str, cwd: Path) -> str:
    result = subprocess.run(["git", *GIT_IDENTITY, *args], cwd=cwd, capture_output=True,
                            text=True, check=False)
    if result.returncode != 0:
        sys.exit(f"git {' '.join(args)} failed in {cwd}:\n{result.stderr.strip()}")
    return result.stdout


class OtherMachine:
    """A plain Git clone of the scenario's remote that edits card files directly."""

    def __init__(self, remote: Path, path: Path) -> None:
        self.path = path
        if path.exists():
            shutil.rmtree(path)
        path.parent.mkdir(parents=True, exist_ok=True)
        git("clone", "--quiet", str(remote), str(path), cwd=path.parent)

    def card_file(self, card_id: str) -> Path:
        for line in git("ls-files", "cards", cwd=self.path).splitlines():
            if line.endswith(f"/{card_id}.md"):
                return self.path / line
        sys.exit(f"Card file for {card_id} is not in the other machine's clone.")

    def _split(self, card_id: str) -> tuple[Path, bytes, bytes]:
        path = self.card_file(card_id)
        raw = path.read_bytes()
        end = raw.find(b"\n---\n", 4)
        if not raw.startswith(b"---\n") or end < 0:
            sys.exit(f"{path} does not start with front matter.")
        return path, raw[: end + 5], raw[end + 5:]

    def set_body(self, card_id: str, body: str) -> None:
        path, front, _ = self._split(card_id)
        path.write_bytes(front + body.encode())

    def set_title(self, card_id: str, title: str) -> None:
        path, front, body = self._split(card_id)
        lines = front.decode().split("\n")
        lines = [f"title: {title}" if line.startswith("title: ") else line for line in lines]
        path.write_bytes("\n".join(lines).encode() + body)

    def trash(self, card_id: str) -> None:
        """Move a card to Trash the way Holder writes it: under trash/, with deleted_at set."""
        path, front, body = self._split(card_id)
        rel = path.relative_to(self.path).as_posix()
        trash_rel = "trash/" + rel
        now = int(time.time())
        lines = []
        for line in front.decode().split("\n"):
            if line.startswith("rel_path: "):
                line = f"rel_path: {trash_rel}"
            elif line.startswith("deleted_at: "):
                line = f"deleted_at: {now}"
            elif line.startswith("updated_at: "):
                line = f"updated_at: {now}"
            lines.append(line)
        (self.path / trash_rel).parent.mkdir(parents=True, exist_ok=True)
        git("mv", rel, trash_rel, cwd=self.path)
        (self.path / trash_rel).write_bytes("\n".join(lines).encode() + body)
        git("add", trash_rel, cwd=self.path)

    def push(self, message: str) -> None:
        git("commit", "--quiet", "-am", message, cwd=self.path)
        result = subprocess.run(["git", *GIT_IDENTITY, "push", "--quiet"], cwd=self.path,
                                capture_output=True, text=True, check=False)
        if result.returncode != 0:
            sys.exit("The other machine's push was rejected. The sandbox daemon probably synced in "
                     "the background first. Run setup again.\n" + result.stderr.strip())


# --- Scenarios -------------------------------------------------------------------------------

@dataclass
class Context:
    project: object
    other: OtherMachine
    cards: dict[str, object]


@dataclass
class Scenario:
    name: str
    summary: str
    build: Callable[[object], dict[str, object]]   # Initial cards, pushed to both sides.
    local: Callable[[Context], None]               # Holder's side (becomes the copy).
    remote: Callable[[Context], None]              # Other machine (wins, stays the original).
    after: Callable[[Context, list], None] | None = None
    copies: int = 1                                # Expected copies; 0 means merged automatically.


PARAGRAPHS = "First paragraph.\n\nSecond paragraph.\n\nThird paragraph.\n"


def one_card(project) -> dict[str, object]:
    return {"card": project.cards.create(title="Meeting notes", content=PARAGRAPHS)}


def basic_local(ctx: Context) -> None:
    ctx.cards["card"].update(content=PARAGRAPHS.replace("Second paragraph.",
                                                        "Second paragraph, rewritten on this machine."))


def basic_remote(ctx: Context) -> None:
    card = ctx.cards["card"]
    ctx.other.set_title(card.id, "Meeting notes (agenda)")
    # The same line as this machine's edit, so the two cannot be merged line by line.
    ctx.other.set_body(card.id, PARAGRAPHS.replace("Second paragraph.",
                                                   "Second paragraph, rewritten on the other machine.")
                       + "\nFourth paragraph, added on the other machine.\n")
    ctx.other.push("Edit Meeting notes elsewhere")


def family(project) -> dict[str, object]:
    parent = project.cards.create(title="Trip plan", content="Where we go.\n")
    child = parent.children.create(title="Packing list", content="- Boots\n- Map\n")
    return {"parent": parent, "child": child}


def family_local(ctx: Context) -> None:
    ctx.cards["parent"].update(content="Where we go: the coast.\n")
    ctx.cards["child"].update(content="- Boots\n- Map\n- Sun cream\n")


def family_remote(ctx: Context) -> None:
    ctx.other.set_body(ctx.cards["parent"].id, "Where we go: the mountains.\n")
    ctx.other.set_body(ctx.cards["child"].id, "- Boots\n- Map\n- Rope\n")
    ctx.other.push("Edit trip plan elsewhere")


def original_under_copy_after(ctx: Context, copies: list) -> None:
    copy_card = ctx.project.cards.get(copies[0].copy_card.card_id)
    original = ctx.project.cards.get(copies[0].original_card_id)
    original.move(parent=copy_card)


def extras(project) -> dict[str, object]:
    return {"card": project.cards.create(title="Reading list", content="Books to read.\n"),
            "target": project.cards.create(title="Library", content="Local library card.\n"),
            "linker": project.cards.create(title="Weekly review", content="Check the reading list.\n")}


def extras_local(ctx: Context) -> None:
    from holder import MilestoneDate  # Imported after connect() fixed sys.path.
    card = ctx.cards["card"]
    card.update(content="Books to read. #books #later\n")
    card.links.add(ctx.cards["target"].id, label="Borrow from")
    card.milestones.create(kind="deadline", description="Return books",
                           dates=[MilestoneDate("due", date(2026, 12, 24))])


def extras_remote(ctx: Context) -> None:
    ctx.other.set_body(ctx.cards["card"].id, "Books to read, sorted by author.\n")
    ctx.other.push("Edit Reading list elsewhere")


def extras_after(ctx: Context, copies: list) -> None:
    # A backlink to the copy itself, so the cleanup review lists one.
    ctx.cards["linker"].links.add(copies[0].copy_card.card_id, label="See also")


def large_build(project) -> dict[str, object]:
    return {"card": project.cards.create(title="Big import", content="placeholder\n")}


def large_local(ctx: Context) -> None:
    ctx.cards["card"].update(content="".join(f"Local line {i}\n" for i in range(1, 1201)))


def large_remote(ctx: Context) -> None:
    ctx.other.set_body(ctx.cards["card"].id, "".join(f"Remote line {i}\n" for i in range(1, 1201)))
    ctx.other.push("Replace Big import elsewhere")


CRLF = "Line one\r\nLine two\r\nLine three\r\nLine four\r\n"


def crlf_build(project) -> dict[str, object]:
    return {"card": project.cards.create(title="Windows notes", content=CRLF)}


def crlf_local(ctx: Context) -> None:
    ctx.cards["card"].update(content=CRLF.replace("Line two", "Line two, edited here"))


def crlf_remote(ctx: Context) -> None:
    ctx.other.set_body(ctx.cards["card"].id, CRLF.replace("Line two", "Line two, edited there")
                       .replace("Line four", "Line four, edited there"))
    ctx.other.push("Edit Windows notes elsewhere")


def move_build(project) -> dict[str, object]:
    return {"card": project.cards.create(title="Loose idea", content="An idea to file.\n"),
            "folder": project.cards.create(title="Ideas", content="Filed ideas.\n")}


def move_local(ctx: Context) -> None:
    ctx.cards["card"].move(parent=ctx.cards["folder"])


def move_remote(ctx: Context) -> None:
    ctx.other.set_body(ctx.cards["card"].id, "An idea to file, with more detail.\n")
    ctx.other.push("Edit Loose idea elsewhere")


SAME_EDIT = PARAGRAPHS.replace("Third paragraph.", "Third paragraph, fixed typo.")


def identical_local(ctx: Context) -> None:
    ctx.cards["card"].update(content=SAME_EDIT)


def identical_remote(ctx: Context) -> None:
    ctx.other.set_body(ctx.cards["card"].id, SAME_EDIT)
    ctx.other.push("Same edit elsewhere")


def title_local(ctx: Context) -> None:
    ctx.cards["card"].update(title="Meeting notes, Monday")


def title_remote(ctx: Context) -> None:
    ctx.other.set_body(ctx.cards["card"].id, PARAGRAPHS + "\nFourth paragraph, from the other machine.\n")
    ctx.other.push("Edit Meeting notes elsewhere")


def lines_local(ctx: Context) -> None:
    ctx.cards["card"].update(content=PARAGRAPHS.replace("First paragraph.", "First paragraph, edited here."))


def lines_remote(ctx: Context) -> None:
    ctx.other.set_body(ctx.cards["card"].id,
                       PARAGRAPHS.replace("Third paragraph.", "Third paragraph, edited there.")
                       + "\nFourth paragraph, added there.\n")
    ctx.other.push("Edit other lines of Meeting notes elsewhere")


def neighbour_local(ctx: Context) -> None:
    ctx.cards["card"].update(content=PARAGRAPHS.replace("Second paragraph.", "Second paragraph, edited here."))


def neighbour_remote(ctx: Context) -> None:
    ctx.other.set_body(ctx.cards["card"].id,
                       PARAGRAPHS.replace("Second paragraph.\n", "Second paragraph.\nA line added just below it.\n"))
    ctx.other.push("Add a line to Meeting notes elsewhere")


def link_local(ctx: Context) -> None:
    ctx.cards["card"].links.add(ctx.cards["target"].id, label="Borrow from")


def both_move_local(ctx: Context) -> None:
    ctx.cards["card"].move(parent=ctx.cards["folder"])


def both_move_remote(ctx: Context) -> None:
    # The other machine moves the card into a different place: after the folder, at the top level.
    path, front, body = ctx.other._split(ctx.cards["card"].id)
    lines = ["sort_key: 99" if line.startswith("sort_key: ") else line
             for line in front.decode().split("\n")]
    path.write_bytes("\n".join(lines).encode() + body)
    ctx.other.push("Move Loose idea elsewhere")


def trash_local(ctx: Context) -> None:
    ctx.cards["card"].trash()


def trash_remote(ctx: Context) -> None:
    ctx.other.set_body(ctx.cards["card"].id, "An idea to file, edited on the other machine.\n")
    ctx.other.push("Edit Loose idea elsewhere")


def edit_local(ctx: Context) -> None:
    ctx.cards["card"].update(content="An idea to file, edited on this machine.\n")


def trashed_remote(ctx: Context) -> None:
    ctx.other.trash(ctx.cards["card"].id)
    ctx.other.push("Delete card Loose idea")


def hamlet_build(project) -> dict[str, object]:
    return {"card": project.cards.create(title="Hamlet", content="To be, or not to be\n")}


def hamlet_local(ctx: Context) -> None:
    # The first quarto's reading.
    ctx.cards["card"].update(content="To be, or not to be, ay, there's the point,\n"
                                     "To die, to sleep, is that all? Ay, all:\n")


def hamlet_remote(ctx: Context) -> None:
    # The Folio's.
    ctx.other.set_body(ctx.cards["card"].id, "To be, or not to be, that is the question:\n"
                                             "Whether 'tis nobler in the mind to suffer\n")
    ctx.other.push("Edit Hamlet elsewhere")


SCENARIOS = [
    Scenario("basic", "Both sides edit the same line; the other machine also renames the card.",
             one_card, basic_local, basic_remote),
    Scenario("hamlet", "A demo: the same line of Hamlet rewritten two ways, each adding a second line.",
             hamlet_build, hamlet_local, hamlet_remote),
    Scenario("children", "A parent and its child conflict, so the copy has a child of its own.",
             family, family_local, family_remote),
    Scenario("original-under-copy", "The original is moved under its copy; Trash is blocked.",
             one_card, basic_local, basic_remote, original_under_copy_after),
    Scenario("copy-extras", "Tags, a link and a milestone only on the copy, plus a backlink to it.",
             extras, extras_local, extras_remote, extras_after),
    Scenario("large-diff", "1,200 different lines each side, too large to highlight (truncated).",
             large_build, large_local, large_remote),
    Scenario("crlf", "CRLF line endings on both sides, to check line highlighting (#481).",
             crlf_build, crlf_local, crlf_remote),
    Scenario("both-move", "Both machines move the card to different places.",
             move_build, both_move_local, both_move_remote),
    Scenario("neighbouring-lines", "This machine edits a line; the other adds a line just below it.",
             one_card, neighbour_local, neighbour_remote),
    # Merged automatically: the two machines changed different parts of the card.
    Scenario("move-vs-edit", "This machine only moves the card; the other edits its text.",
             move_build, move_local, move_remote, copies=0),
    Scenario("identical-edit", "Both machines make the exact same text edit.",
             one_card, identical_local, identical_remote, copies=0),
    Scenario("title-vs-body", "This machine renames the card; the other adds a paragraph.",
             one_card, title_local, title_remote, copies=0),
    Scenario("different-lines", "Both machines edit the text, on different lines.",
             one_card, lines_local, lines_remote, copies=0),
    Scenario("link-vs-edit", "This machine adds a link; the other edits the text.",
             extras, link_local, extras_remote, copies=0),
    Scenario("trash-vs-edit", "This machine moves the card to Trash; the other edits it. "
             "The edited card stays live.", move_build, trash_local, trash_remote, copies=0),
    Scenario("edit-vs-trash", "This machine edits the card; the other moves it to Trash. "
             "The card is restored with the edit.", move_build, edit_local, trashed_remote,
             copies=0),
]
BY_NAME = {s.name: s for s in SCENARIOS}


def run_scenario(holder, root: Path, scenario: Scenario, pending: bool = False) -> bool:
    name = f"Conflict: {scenario.name}"
    for existing in holder.projects.filter(name=name):
        if existing.name == name:
            sys.exit(f"Project '{name}' already exists. Run reset first, or pick other scenarios.")

    remote = root / "remotes" / f"{scenario.name}.git"
    if remote.exists():
        shutil.rmtree(remote)
    remote.mkdir(parents=True)
    git("init", "--quiet", "--bare", str(remote), cwd=remote.parent)

    project = holder.projects.create(name=name, privacy_mode="plain")
    cards = scenario.build(project)
    project.git.set_remote(str(remote))
    # Push (the empty remote has no branch to pull yet), then pull, so the background worker's
    # first pull is not due while the edits below are made.
    for step in (project.git.push, project.git.pull):
        outcome = step()
        if not outcome.succeeded:
            sys.exit(f"[{scenario.name}] initial {step.__name__} failed: "
                     f"{outcome.pull.error_message or outcome.push.error_message}")

    other = OtherMachine(remote, root / "other" / scenario.name)
    ctx = Context(project, other, cards)
    scenario.local(ctx)    # Committed here, not pushed.
    scenario.remote(ctx)   # Committed and pushed from the other clone.
    if pending:
        # Left for Desktop's Sync now or the next background pull.
        note = "  (its extra step runs only without --pending)" if scenario.after else ""
        print(f"  {scenario.name:<20} {'waiting for a pull':<21} {scenario.summary}{note}")
        return True
    result = project.git.pull()
    if not result.succeeded:
        print(f"  {scenario.name:<20} {'PULL FAILED':<21} {scenario.summary}\n"
              f"  {'':<20} {result.pull.error_message or result.pull.status}")
        return False
    made = result.pull.conflicts_resolved
    copies = list(project.git.status().conflict_copies or [])
    if scenario.after and copies:
        scenario.after(ctx, copies)
    outcome = "merged automatically" if made == 0 else f"{made} conflict cop{'y' if made == 1 else 'ies'}"
    expected = made == scenario.copies or (scenario.copies > 0 and made > 0)
    flag = "" if expected else f"  << UNEXPECTED, wanted {'a merge' if scenario.copies == 0 else 'a copy'}"
    print(f"  {scenario.name:<20} {outcome:<21} {scenario.summary}{flag}")
    return expected


# --- Commands --------------------------------------------------------------------------------

def cmd_setup(args: argparse.Namespace) -> None:
    unknown = [n for n in args.scenarios if n not in BY_NAME]
    if unknown:
        sys.exit(f"Unknown scenario(s): {', '.join(unknown)}. See: sandbox.py list")
    chosen = [BY_NAME[n] for n in args.scenarios] or SCENARIOS
    if shutil.which("git") is None:
        sys.exit("git is not on PATH.")
    holderctl, holderd = daemon_binaries(args)
    root = args.root
    print(f"Sandbox: {root}\nholderctl: {holderctl}\nholderd: {holderd}")
    start_daemon(root, holderctl, holderd, args.port)
    print("Scenarios:")
    with connect(holderctl) as holder:
        unexpected = [s.name for s in chosen if not run_scenario(holder, root, s, args.pending)]
    print("\nOpen Desktop on the sandbox with:\n"
          f"  python3 {Path(__file__).relative_to(REPO)} desktop"
          + ("" if root == DEFAULT_ROOT else f" --root {root}"))
    if args.pending:
        print("\nBoth machines have edited, and nothing is pulled yet. Open a 'Conflict: …' project "
              "and click Sync now in Git Sync, or leave it open for the background pull (within "
              "about five minutes of now). The conflict appears with its toast.")
    if unexpected:
        sys.exit(f"\nUnexpected outcome for: {', '.join(unexpected)}. The daemon may predate or "
                 "postdate the merge rules this script expects.")


def cmd_desktop(args: argparse.Namespace) -> None:
    holderctl, holderd = daemon_binaries(args)
    start_daemon(args.root, holderctl, holderd, args.port)
    build = REPO / "apps" / "desktop" / "gtk" / "build"
    desktop = find_binary("desktop", args.desktop, "HOLDER_SANDBOX_DESKTOP",
                          [build / f"holder-desktop{EXE}", build / "src" / f"holder-desktop{EXE}"])
    print(f"Starting {desktop}. Open a 'Conflict: …' project, then Git Sync → Compare versions.")
    subprocess.run([str(desktop)], env=desktop_env(desktop), check=False)


def desktop_env(desktop: Path) -> dict[str, str]:
    """Desktop's environment: the sandbox's, with this build's settings schema and the sandbox's
    own settings. A Holder schema installed elsewhere may be older than the build, and GLib aborts
    when the build asks it for a key it lacks; GSETTINGS_SCHEMA_DIR is searched first. The keyfile
    backend keeps preferences under the sandbox's config folder, so they never touch your own
    Desktop's and reset deletes them."""
    env = os.environ.copy()
    for data in (desktop.parent / "data", desktop.parent.parent / "data"):
        if (data / "gschemas.compiled").is_file():
            existing = env.get("GSETTINGS_SCHEMA_DIR")
            env["GSETTINGS_SCHEMA_DIR"] = str(data) + (os.pathsep + existing if existing else "")
            break
    env["GSETTINGS_BACKEND"] = "keyfile"
    return env


def cmd_env(args: argparse.Namespace) -> None:
    holderctl = None
    try:
        holderctl, _ = daemon_binaries(args)
    except SystemExit:
        pass
    for key, value in sandbox_env(args.root, holderctl).items():
        if os.name == "nt":
            print(f'$env:{key} = "{value}"')
        else:
            print(f"export {key}='{value}'")


def cmd_stop(args: argparse.Namespace) -> None:
    stop_daemon(args.root)


def cmd_reset(args: argparse.Namespace) -> None:
    root = args.root.resolve()
    if not root.exists():
        print(f"Nothing to reset at {root}.")
        return
    if not (root / ".holder-conflict-sandbox").is_file():
        sys.exit(f"{root} is not a sandbox made by this script; not deleting it.")
    stop_daemon(root)
    shutil.rmtree(root)
    print(f"Deleted {root}.")


def cmd_list(_: argparse.Namespace) -> None:
    for scenario in SCENARIOS:
        print(f"{scenario.name:<20} {scenario.summary}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--root", type=Path, default=DEFAULT_ROOT,
                        help=f"sandbox folder (default {DEFAULT_ROOT})")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT, help="sandbox daemon port")
    parser.add_argument("--holderctl", help="holderctl binary (default: daemon/build)")
    parser.add_argument("--holderd", help="holderd binary (default: daemon/build)")
    parser.add_argument("--desktop", help="Desktop binary (default: apps/desktop/gtk/build)")
    commands = parser.add_subparsers(dest="command", required=True)
    setup = commands.add_parser("setup", help="start the sandbox daemon and build scenarios")
    setup.add_argument("scenarios", nargs="*", help="scenario names (default: all)")
    setup.add_argument("--pending", action="store_true",
                       help="stop before the pull, so it happens with Desktop open and shows its toast")
    setup.set_defaults(run=cmd_setup)
    commands.add_parser("desktop", help="open Desktop on the sandbox").set_defaults(run=cmd_desktop)
    commands.add_parser("env", help="print environment exports for your shell").set_defaults(run=cmd_env)
    commands.add_parser("stop", help="stop the sandbox daemon").set_defaults(run=cmd_stop)
    commands.add_parser("reset", help="stop the daemon and delete the sandbox").set_defaults(run=cmd_reset)
    commands.add_parser("list", help="list scenarios").set_defaults(run=cmd_list)
    args = parser.parse_args()
    args.root = args.root.expanduser().resolve()
    if args.command in ("setup", "desktop"):
        args.root.mkdir(parents=True, exist_ok=True)
        (args.root / ".holder-conflict-sandbox").touch()
    args.run(args)


if __name__ == "__main__":
    main()
