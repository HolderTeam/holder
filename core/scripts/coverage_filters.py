"""Remove verified compiler-only function aliases without changing line coverage.

GCC emits an Itanium ABI D0 deleting destructor as well as the D2 body. The
abstract interfaces below cannot be constructed, and their private callback
adapters are created with make_shared, which destroys the body in place. D0 is
therefore unused. Keep this list narrow: other destructors can have real delete
paths. Require a covered matching D2 body before dropping a zero-hit D0 record.
"""

from __future__ import annotations

import argparse
from pathlib import Path


DELETING_ALIASES = {
    "src/api/Contract.h": {
        "_ZN6holder8contract9SshSignerD0Ev",
        "_ZN6holder8contract15StorageProviderD0Ev",
        "_ZN6holder8contract16TimeZoneProviderD0Ev",
        "_ZN6holder8contract15KeyringProviderD0Ev",
    },
    "src/api/HostCApi.cpp": {
        "_ZN12_GLOBAL__N_125CStorageProviderCallbacksD0Ev",
        "_ZN12_GLOBAL__N_126CTimeZoneProviderCallbacksD0Ev",
        "_ZN12_GLOBAL__N_125CKeyringProviderCallbacksD0Ev",
        "_ZN12_GLOBAL__N_119CSshSignerCallbacksD0Ev",
    },
    "src/platform/time_zone/TimeZones.h": {
        "_ZN6holder8platform16TimeZoneProviderD0Ev",
    },
}


def filter_record(lines: list[str]) -> tuple[list[str], int]:
    source = next((line[3:].replace("\\", "/") for line in lines if line.startswith("SF:")), "")
    allowed = next((names for path, names in DELETING_ALIASES.items()
                    if source == path or source.endswith("/" + path)), set())
    modern = any(line.startswith("FNA:") for line in lines)
    hits: dict[str, int] = {}
    for line in lines:
        if modern and line.startswith("FNA:"):
            _, count, name = line[4:].split(",", 2)
            hits[name] = int(count)
        elif not modern and line.startswith("FNDA:"):
            count, name = line[5:].split(",", 1)
            hits[name] = int(count)
    removed = {name for name in allowed if hits.get(name) == 0
               and hits.get(name.replace("D0Ev", "D2Ev"), 0) > 0}
    if not removed:
        return lines, 0
    retained_ids = {line[4:].split(",", 1)[0] for line in lines
                    if line.startswith("FNA:") and line[4:].split(",", 2)[2] not in removed}
    hit_ids = {line[4:].split(",", 1)[0] for line in lines
               if line.startswith("FNA:") and int(line[4:].split(",", 2)[1]) > 0
               and line[4:].split(",", 2)[2] not in removed}
    output = []
    for line in lines:
        if line.startswith("FNA:") and line[4:].split(",", 2)[2] in removed:
            continue
        if line.startswith("FNL:") and line[4:].split(",", 1)[0] not in retained_ids:
            continue
        if line.startswith("FN:") and line.rsplit(",", 1)[-1] in removed:
            continue
        if line.startswith("FNDA:") and line[5:].split(",", 1)[1] in removed:
            continue
        if line.startswith("FNF:"):
            line = f"FNF:{len(retained_ids) if modern else len(hits) - len(removed)}"
        elif line.startswith("FNH:"):
            covered = len(hit_ids) if modern else sum(count > 0 for name, count in hits.items() if name not in removed)
            line = f"FNH:{covered}"
        output.append(line)
    return output, len(removed)


def filter_lcov(text: str) -> tuple[str, int]:
    output: list[str] = []
    record: list[str] = []
    removed = 0
    for line in text.splitlines():
        record.append(line)
        if line == "end_of_record":
            filtered, count = filter_record(record)
            output.extend(filtered)
            removed += count
            record = []
    if record:
        filtered, count = filter_record(record)
        output.extend(filtered)
        removed += count
    return "\n".join(output) + ("\n" if text.endswith("\n") else ""), removed


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("tracefile", type=Path)
    args = parser.parse_args()
    original = args.tracefile.read_text(encoding="utf-8")
    filtered, count = filter_lcov(original)
    if count:
        args.tracefile.write_text(filtered, encoding="utf-8")
    print(f"Excluded {count} verified compiler deleting-destructor function aliases; line counters unchanged.")


if __name__ == "__main__":
    main()
