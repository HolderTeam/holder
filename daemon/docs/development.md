# Developing the daemon

Holderd owns database access, authentication, HTTP validation, concurrency,
background scheduling and API presentation. Reusable platform-independent
storage and domain behavior belongs in [core](../../core/README.md). Clients
use core's domain behavior through the supported API rather than duplicating
database, Git or Markdown mutation policy.

Shared `boost::asio::io_context` instances belong to dedicated I/O threads.
Request workers must not call `run()`, `run_one()`, `poll()` or related methods
on a shared context while waiting for socket work. Keep request data alive for
the full operation and keep database access with the worker that owns it.
Investigate intermittent memory corruption, iterator invalidation and
concurrency failures as defects.

When changing an HTTP route (see [`contract/HTTP.md`](../../contract/HTTP.md)),
change the Python contract and run `make -C contract update`, which writes the
canonical [`api/openapi.yaml`](../../api/openapi.yaml). Update the daemon handler
and its tests in the same pull request. Follow
[Changing the contract](../../api/README.md#changing-the-contract) for version
bumps and changes in meaning.

For core changes needed by the daemon, update core and its tests in the same
pull request. The daemon uses only core's installed C++ headers, CMake package,
schema and welcome resources. Do not refer to files in `../core` by path from
daemon code, CMake or scripts. The [core build guide](core-builds.md) explains
local and CI dependency selection. Core owns its tests and sanitizer coverage;
the daemon owns its HTTP and integration tests.
