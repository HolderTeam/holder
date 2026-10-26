"""python -m holder_contract: check the contract, write its IR, or generate code from it."""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

from .daemon import generate_daemon
from .extract import ContractError, extract
from .generate import CORE_PATHS, Unsupported, generate

CONTRACT = Path(__file__).resolve().parents[1]
REPOSITORY = CONTRACT.parent
COMMANDS = {
    "check": "report every problem in the contract, and fail if core's generated files are out of date",
    "ir": "write the contract's IR as JSON",
    "generate": "write every generated file into --output, a directory: core's C API, and the daemon's routes, "
                "their JSON codec and their OpenAPI paths",
    "update": "write the generated C API into core",
}


def collisions(files: dict[str, str]) -> list[str]:
    """Generated C names that the hand-written part of holder.h already uses."""
    header = (REPOSITORY / "core/include/holder/holder.h").read_text(encoding="utf-8")
    header = header.split('#include "holder_api.h"')[0]
    existing = set(re.findall(r"\bholder_\w+", header))
    generated = set(re.findall(r"(?:typedef (?:struct|enum) |int )(holder_\w+)", files["holder_api.h"]))
    return sorted(generated & existing)


def stale_files(files: dict[str, str]) -> list[str]:
    stale = []
    for name, text in files.items():
        path = REPOSITORY / CORE_PATHS[name]
        if not path.exists() or path.read_text(encoding="utf-8") != text:
            stale.append(CORE_PATHS[name])
    return stale


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(prog="python -m holder_contract", description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter,
                                     epilog="\n".join(f"  {name}: {text}" for name, text in COMMANDS.items()))
    parser.add_argument("command", choices=list(COMMANDS))
    parser.add_argument("--root", type=Path, default=CONTRACT, help="the directory holding the contract package")
    parser.add_argument("--package", default="interface", help="the contract package's name")
    parser.add_argument("--output", type=Path, help="ir: a file instead of standard output; generate: a directory")
    args = parser.parse_args(argv)
    try:
        ir = extract(args.root, args.package)
    except ContractError as error:
        print(error, file=sys.stderr)
        print(f"{len(error.problems)} problem(s) in the contract.", file=sys.stderr)
        return 1
    if args.command == "ir":
        text = json.dumps(ir, indent=2, ensure_ascii=False) + "\n"
        if args.output:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.write_text(text, encoding="utf-8")
        else:
            sys.stdout.write(text)
        return 0
    if args.command == "check" and (args.root.resolve(), args.package) != (CONTRACT, "interface"):
        return 0
    try:
        files = generate(ir)
    except Unsupported as error:
        print(f"The C generator does not support this yet: {error}", file=sys.stderr)
        return 1
    clashes = collisions(files)
    if clashes:
        print(f"Generated names already used in core/include/holder/holder.h: {', '.join(clashes)}. "
              "Rename the contract's declaration.", file=sys.stderr)
        return 1
    if args.command == "check":
        stale = stale_files(files)
        for path in stale:
            print(f"{path} is out of date; run make -C contract update.", file=sys.stderr)
        return 1 if stale else 0
    if args.command == "generate":
        if not args.output:
            parser.error("generate needs --output, a directory")
        try:
            files = {**files, **generate_daemon(ir)}
        except Unsupported as error:
            print(f"The daemon's generators do not support this yet: {error}", file=sys.stderr)
            return 1
        targets = {name: args.output / name for name in files}
    else:
        targets = {name: REPOSITORY / path for name, path in CORE_PATHS.items()}
    for name, text in files.items():
        targets[name].parent.mkdir(parents=True, exist_ok=True)
        targets[name].write_text(text, encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
