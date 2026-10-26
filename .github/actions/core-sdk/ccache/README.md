# Core compiler caches

Core CI and Core / SDK use ccache through CMake's C++ compiler launcher. Tests
and SDKs have separate caches, further separated by runner OS/architecture,
compiler binary, ccache version, checkout/compiler paths, and build type.
Ccache also checks compiler content, arguments, source and included headers.
A moved checkout intentionally misses rather than rewriting absolute fixture
paths or Windows vcpkg include paths. MSVC uses embedded debug information
(`/Z7` via CMP0141) so compile results can be cached; executable PDBs remain.

Only successful non-PR runs on `main` save compiler caches. PRs restore main's
compatible snapshot. Each cache is capped at 500 MB, and a successful save
prunes older run snapshots for that compiler/configuration using the existing
pruning script. Finished SDK lookup by core tree is unchanged; compiler caches
are used only when compilation is necessary. SDK artifacts and required result
check names are unchanged.

## Measurement

Dispatch **Core / CI** with `measure_cache=true` on the candidate branch. All
three test configurations and SDK platforms build with isolated empty caches,
then clean-rebuild with a temporary `static_assert` appended to one compiled
core source. This forces a real source miss while unchanged translation units
can hit. The source is restored in a `finally` block. The normal tests, install,
installed-consumer checks and SDK packaging follow. Measurement compiler/SDK
caches are not saved.

The log and job summary report cold/warm build seconds and cache hits/misses;
the benchmark fails if it sees no cold misses, no warm hits, or no changed-source
miss. Timings include compilation and linking, not dependency installation or
runner queue time. Use whole-job duration as well when assessing time to result.
Linux coverage uses different instrumentation and follows the regular tests.

## Verified results

[Core / CI run 40318512084](https://github.com/HolderTeam/holder/actions/runs/40318512084)
on commit `2d417d1a` passed all six platform/configuration measurements, core
tests, Linux coverage, installed-consumer checks, SDK packaging and the required
Core CI result. Windows compile commands were checked for embedded debug flags.

| Build | Cold seconds | Warm seconds, one changed source | Warm hits / misses |
| --- | ---: | ---: | ---: |
| linux test (RelWithDebInfo) | 232.65 | 3.72 | 211 / 2 |
| linux sdk (RelWithDebInfo) | 215.97 | 3.70 | 138 / 1 |
| macos test (RelWithDebInfo) | 136.34 | 3.38 | 137 / 1 |
| macos sdk (RelWithDebInfo) | 154.10 | 5.64 | 137 / 1 |
| windows test (Debug) | 188.94 | 10.08 | 137 / 1 |
| windows sdk (RelWithDebInfo) | 338.86 | 8.88 | 137 / 1 |

These are build-and-link times on hosted runners, not whole-pipeline speedups.
Dependency installation, test execution and runner queueing remain. Linux's
shared/static build also gets some hits within the cold build itself. The warm
source edit was `static_assert(true, ...)` in `src/platform/Tx.cpp`.

The six final local compiler caches occupied approximately 0.52 GB in total
(including Linux coverage), well below their combined caps. This does not yet
measure cross-run cache residency: compiler snapshots start being saved on
successful main runs after merge. Monitor those restores and eviction before
considering remote storage. Release SDKs use the same launcher configuration;
the measured SDK configuration here was RelWithDebInfo.

## Capacity

On 26 October 2026, the repository held 52 Actions caches totalling
10,114,291,475 bytes (about 9.4 GiB), including approximately 4.5 GB of daemon
compiler caches and many old backend snapshots. Use the free 10 GB repository budget as the working limit; no paid storage or
remote service is enabled by this change. The storage-limit and retention-limit
APIs returned HTTP 402, so a custom limit could not be confirmed. GitHub's
[documented defaults](https://docs.github.com/en/actions/reference/workflows-and-actions/dependency-caching)
are 10 GB total and eviction after seven days without access.

The new core compiler caches
are capped at 500 MB each (three test configurations plus the SDK platform/build
type combinations actually used). This adds eviction pressure; inspect
`gh api repos/HolderTeam/holder/actions/cache/usage` and actual saved sizes after
main runs. Older compiler versions also age out under GitHub's cache retention.
Do not introduce a remote cache by default: first measure hit rates, eviction
and time saved. If useful core snapshots cannot remain resident, coordinate
retention/budget changes with the other CI caches before choosing remote storage.
