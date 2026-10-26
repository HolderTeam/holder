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

## Capacity

On 26 October 2026, the repository held 52 Actions caches totalling
10,114,291,475 bytes (about 9.4 GiB), including approximately 4.5 GB of daemon
compiler caches and many old backend snapshots. The new core compiler caches
are capped at 500 MB each (three test configurations plus the SDK platform/build
type combinations actually used). This adds eviction pressure; inspect
`gh api repos/HolderTeam/holder/actions/cache/usage` and actual saved sizes after
main runs. Older compiler versions also age out under GitHub's cache retention.
Do not introduce a remote cache by default: first measure hit rates, eviction
and time saved. If useful core snapshots cannot remain resident, coordinate
retention/budget changes with the other CI caches before choosing remote storage.
