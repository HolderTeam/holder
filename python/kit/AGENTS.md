# Coding-agent instructions

Holder Kit is the CPython binding over libholder: `src/native/module.c` wraps core's C API and
`src/holderkit` is the Python layer over it. Reusable storage and domain behaviour belongs in core
(`../../core`), not here; Kit exposes it to Python.

# Core

Core lives in `../../core` in this repository. Make core changes there, with core's own tests, in
the same pull request as the Kit changes that need them.

Kit uses only core's stable C API (`#include <holder/holder.h>`), through what core installs: that
header, the `holder` CMake package and the schema. Do not include core's other headers, and do not
refer to files in `../../core` by path from Kit's code, CMake or scripts. CI enforces this: it builds
Kit against the SDK that `Core / SDK` builds from `core/` in the same commit, and a boundary check
rejects other core headers and paths into core.

`./make.sh` builds `../../core` from source with Kit for local development (or
`HOLDER_KIT_CORE_SOURCE`). `./make.sh --sdk` with `HOLDER_CORE_SDK` builds against a prepared SDK
instead.
