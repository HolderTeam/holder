# Coding-agent instructions

Treat the current code, tests, OpenAPI contract, and active task plan as authoritative. Other planning or design documents may be stale unless the current task explicitly references them.

Holderd owns database access, authentication, HTTP validation, concurrency, background scheduling, and API presentation. Reusable non-platform-specific domain behavior belongs in core (`../core`).

Clients must not duplicate database, Git, Markdown-mutation, or other domain policy implemented in core.

Shared `boost::asio::io_context` instances are owned by dedicated I/O threads. Request workers must not call `run()`, `run_one()`, `poll()`, or similar methods on a shared I/O context while waiting for request socket work.

Preserve request-data lifetimes and worker-owned database access. Treat intermittent memory corruption, iterator invalidation, and concurrency failures as real defects, not CI noise.

When changing an HTTP route, update `api/openapi.yaml` (at the repository root) and test it in the same change where applicable.

# Core dependency selection

Core lives in `../core` in this repository. Make core changes there, with core's own
tests, in the same pull request as the daemon changes that need them.

The daemon uses core only through what core installs: its headers (installed under
`include/holder`, included here as for example `"card/CardStore.h"`), its `holder`
CMake package, and its schema and welcome resources. Do not refer to files in
`../core` by path from daemon code, CMake or scripts. CI enforces this: it builds the
daemon against the SDK that `Core / SDK` builds from `core/` in the same commit, so
anything core does not install is not found, and a boundary check rejects paths
across the boundary in either direction.

`./make.sh` builds `../core` from source with the daemon for local development.
`HOLDER_CORE_SDK` selects an installed SDK instead, and `HOLDER_USE_SYSTEM_CORE`
the packaged `libholder-dev` and `libholder0` (as Ubuntu source packages do). An
SDK's build configuration must match the daemon's.

Core owns its tests and sanitizer coverage; the daemon owns its HTTP and integration
tests.
