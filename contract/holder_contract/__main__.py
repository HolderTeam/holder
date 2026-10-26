"""python -m holder_contract: check the contract, or write its IR."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

from .extract import ContractError, extract

CONTRACT = Path(__file__).resolve().parents[1]


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(prog="python -m holder_contract", description=__doc__)
    parser.add_argument("command", choices=["check", "ir"],
                        help="check: report every problem in the contract; ir: write its IR as JSON")
    parser.add_argument("--root", type=Path, default=CONTRACT, help="the directory holding the contract package")
    parser.add_argument("--package", default="interface", help="the contract package's name")
    parser.add_argument("--output", type=Path, help="write the IR here instead of to standard output")
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


if __name__ == "__main__":
    sys.exit(main())
