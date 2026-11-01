"""python -m holder_contract: check the contract, write its IR, or generate code from it."""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

from pydantic import ValidationError

from compiler.errors import ContractError
from compiler.ir import ContractView
from compiler.pipeline import Compiler, ContractSource
from generators.android.client import generate_android_client
from generators.common import Unsupported

from .daemon import DAEMON_PATHS, OPENAPI_PATHS, api_documents, generate_daemon
from .extract import load_contract
from .generate import CORE_PATHS, generate

CONTRACT = Path(__file__).resolve().parents[1]
REPOSITORY = CONTRACT.parent
COMMANDS = {
    "check": "report every problem in the contract, and fail if a generated file in core, Android, the daemon or api/ is "
             "out of date",
    "ir": "write the contract's IR as JSON",
    "generate": "write every generated file into --output, a directory: core's C API, Android's Card and Project bridges, "
                "the daemon's routes, their JSON codec and their OpenAPI paths",
    "update": "write the generated files into core, Android, the daemon and api/",
}


def collisions(files: dict[str, str]) -> list[str]:
    """Generated C names that the hand-written part of holder.h already uses."""
    header = (REPOSITORY / "core/include/holder/holder.h").read_text(encoding="utf-8")
    header = header.split('#include "holder_api.h"')[0]
    existing = set(re.findall(r"\bholder_\w+", header))
    generated = set(re.findall(r"(?:typedef (?:struct|enum) |int )(holder_\w+)", files["holder_api.h"]))
    return sorted(generated & existing)


def repository_files(ir: dict, contract, files: dict[str, str]) -> dict[str, str]:
    """Every generated file the repository holds, by its path in the repository."""
    daemon = generate_daemon(ir, contract)
    return {**{CORE_PATHS[name]: text for name, text in files.items()},
            **generate_android_client(ContractView.model_validate(ir)),
            **{DAEMON_PATHS[name]: daemon[name] for name in DAEMON_PATHS},
            **{OPENAPI_PATHS[name]: text for name, text in api_documents(contract, REPOSITORY).items()}}


def stale_files(files: dict[str, str]) -> list[str]:
    stale = []
    for path, text in files.items():
        target = REPOSITORY / path
        if not target.exists() or target.read_text(encoding="utf-8") != text:
            stale.append(path)
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
        ir = Compiler().compile(ContractSource(args.root, args.package)).model_dump(exclude_unset=True)
    except ContractError as error:
        print(error, file=sys.stderr)
        print(f"{len(error.problems)} problem(s) in the contract.", file=sys.stderr)
        return 1
    except ValidationError as error:
        print(f"The extractor produced invalid IR: {error}", file=sys.stderr)
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
    if args.command in ("check", "update"):
        try:
            files = repository_files(ir, load_contract(args.root, args.package), files)
        except (Unsupported, ValueError) as error:
            print(f"A generated file cannot be written: {error}", file=sys.stderr)
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
            files = {**files, **generate_daemon(ir, load_contract(args.root, args.package)),
                     **{path.rsplit('/', 1)[-1]: text
                        for path, text in generate_android_client(ContractView.model_validate(ir)).items()}}
        except (Unsupported, ValueError) as error:
            print(f"A generator does not support this yet: {error}", file=sys.stderr)
            return 1
        targets = {name: args.output / name for name in files}
    else:
        targets = {path: REPOSITORY / path for path in files}
    for name, text in files.items():
        targets[name].parent.mkdir(parents=True, exist_ok=True)
        targets[name].write_text(text, encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
