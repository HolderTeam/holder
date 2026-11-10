# Core coverage audit

Baseline: 15,010 / 15,135 lines (99.2%) and 1,881 / 1,881 functions (100%).
The table below records the 125 missed lcov records before this audit. Line numbers
refer to that baseline; generated output may move when regenerated. gcovr classified
72 of these records as executable. An unhit source record alone does not establish
an untested behavior, and a difficult failure path is not automatically an artifact.

## Final result and measurement scope

Verified on 2026-11-10: **15,055 / 15,055 measured lines (100%)** and
**1,881 / 1,881 functions (100%)**. gcovr has no remaining unexcluded executable
line misses. All 890 CTest entries, 22 generator tests and eight coverage-filter
tests pass. Generated-file freshness, Ruff, clang-format for the changed C++ test,
and `git diff --check` pass.

The last 33 records were resolved in one batch. A final trace comparison proves:

- `Rebuild.cpp:36` is reached by new boundary tests (four hits).
- `ProjectMaintenance.cpp:42` is normalized from zero to the lambda's recorded
  eight entry hits, retaining the function and every other body/branch counter.
- Exactly 31 other zero-hit line records are excluded as detailed below.
- No function symbols were removed or added. Every retained line is covered.

The full core suite ran once for this batch. Reporting was regenerated from its
completed profiles after discovering that a source annotation on the lambda's
entry also erased its function record. That annotation was removed, and an exact
file/symbol normalization replaced it. Tests cover modern and legacy lcov formats,
zero function hits, wrong symbols/files, missing function records, already-positive
line counts, idempotence, and preservation of other missed lines and branches.
Logs are `build-coverage/final-round-coverage.log`,
`build-coverage/final-report-verification.log` and
`build-coverage/final-generator-tests.log`.

**100% refers to this documented measurement scope**, not every hypothetical
resource failure. Defensive guards remain in the executable. In particular, the
full-collection sort-key precision limit is an explicit resource-boundary
exclusion, and the JSON allocation-during-destruction limitation described below
remains unresolved. No malformed input, unsafe pointer, fabricated enormous
container or machine-wide memory exhaustion was used to exercise a guard.

## Result after the repository/parser allocation batch

After the repository/parser allocation batch: **15,053 / 15,086 lines (99.8%)**
and **1,881 / 1,881 functions (100%)**. There are **33 missed lcov records**
left, including **28 executable misses in gcovr**.

All **889 CTest entries pass**, including the new runner's two test cases and
235 assertions. The five coverage-filter tests pass. The new file passes
clang-format, and `git diff --check` passes. Verification completed on
2026-11-10; the log is `build-coverage/read-allocation-verification.log`.

Compared with the previous inventory, `card/CardRepo.cpp:383` is now covered and
no new missed records appear. The two newly measured parser allocation-handler
records are covered. No source exclusions were added.

## Result after the registration-lock batch

After the registration-lock batch: **15,050 / 15,084 lines (99.8%)** and
**1,881 / 1,881 functions (100%)**. There are **34 missed lcov records** left,
including **29 executable misses in gcovr**.

All **888 CTest entries pass**, including the new isolated runner with 83
assertions across three service sections. The five coverage-filter tests pass.
The new file passes clang-format, and `git diff --check` passes. The final
libstdc++ configuration check succeeds and its focused CTest entry passes again.
One full coverage run validates this batch.

The before/after traces contain exactly the same line records. Nine records in
`api/HostCApi.cpp` changed from zero to positive hits: 189–191, 215–217 and 280–282.
Every other record retains its covered/uncovered status. This batch adds tests
and documentation only, without production source changes or new exclusions.

## Result after the crypto dependency batch

After the crypto dependency batch: **15,041 / 15,084 lines (99.7%)** and
**1,881 / 1,881 functions (100%)**. There are **43 missed lcov records** left,
including **38 executable misses in gcovr**.

All **887 CTest entries pass**, including the separate crypto runner with three
test cases and 86 assertions. The five coverage-filter tests also pass. The new
C++ test file passes clang-format, and `git diff --check` passes. One complete
coverage run validates this batch.

A trace comparison accounts for the change exactly: eight zero-hit private
fallback records were excluded, two existing recovery translation records became
covered, and five previously excluded crypto handler records became measured and
covered. Every other record retains its covered/uncovered status after adjusting
for the one-line condition-formatting change in `CryptoService.cpp`.

## Result after the compiler-cleanup review

After the compiler-cleanup review: **15,034 / 15,087 lines (99.6%)**, with
**1,881 / 1,881 functions (100%)**. There are **53 missed lcov records** left,
including **48 executable misses in gcovr**. This batch changes reporting only;
it does not claim new behavioral coverage. A trace comparison verified that the
only removed records are the 29 annotated braces and that every other line keeps
its previous covered/uncovered status.

All 886 core tests, five coverage-filter tests and 21 generator tests pass. The
strengthened exclusion-policy test, generated-file freshness, Ruff and
`git diff --check` also pass. One complete core coverage run validates this batch.

## Result of the initial audit

On 2026-11-09, all 886 core tests, five coverage-filter tests and 21 generator
tests passed. Generated-file freshness, Ruff and shell syntax checks passed.
The corrected lcov capture reports **15,034 / 15,116 lines (99.5%)** and
**1,881 / 1,881 functions (100%)**. This leaves 82 missed lcov records; gcovr marks
48 remaining executable line records as missed after respecting exclusions.

The reduction from 125 missed records is: five lines reached by the two new tests,
19 internal enum guard lines excluded, and 19 falsely zeroed records restored to
their positive gcov counts. No return statements or whole functions were excluded.
The full suite ran once; the capture correction was validated by regenerating the
report from its completed profiles, without repeating the test suite.

## Tested in this batch

- `api/Cards.cpp:149`: create a card beneath an existing cyclic parent chain.
  `CApiMaintenance_test.cpp` checks INVALID_ARGUMENT, a null result, unchanged card
  count and unchanged Git HEAD. The cycle is constructed in the database; public
  card creation still performs its normal validation.
- `api/Git.cpp:35,37–39`: use libgit2's allocator API to fail its home-path allocation.
  `CApiAllocation_test.cpp` checks that the failure is injected, translated to a
  runtime error and accompanied by a destroyable C error. This runs in the existing
  isolated allocation-test executable. The directory name exceeds the existing
  buffer capacity; short names can reuse capacity and miss the allocation entirely.
  libgit2 retains an OOM sentinel in this global buffer, so recovery is not asserted.

## Excluded in this batch

Nineteen output-enum fallback throws in generated `api/ContractCApi.cpp` convert
internal C++ enums to C enums. Every declared enumerator has an explicit case.
These defaults require an invalid internal enum value; passing arbitrary integers
through the public C API tests a separate input conversion, whose validation stays
measured. The generator emits a single-line exclusion on each fallback throw.
A generator regression test checks their exact scope. No function is excluded.

## Remaining guard and dependency review

These records remain visible. The following are call-path findings, not blanket
exclusion permission:

| Area | Baseline records | Finding |
|---|---|---|
| Host registration | `HostCApi.cpp:188–193,214–219,246–251,279–284` | Allocation failures are tested. Registration stores callbacks without invoking them. Other catches protect future adapters and platform resource failures; retain them in the report pending a supported failure seam. |
| Card placement | `Cards.cpp:198,200` | Input intent is validated before core placement. The private translator retains an invalid-intent fallback for future callers. |
| History output enums | `History.cpp:52,64,194` | Exhaustive private output mappings. Invalid persisted/input values must be tested at their validation boundaries rather than fabricated internal values. |
| Project-history privacy | `History.cpp:145–147` | Current optional metadata enrichment catches privacy failures before this outer translator. Preserve the fallback. Card-history key failures are reachable and already tested. |
| Recovery output enums | `RecoverySnapshots.cpp:56,72,87,102` | Private serializers receive validated values. Invalid persisted snapshots are rejected earlier. |
| Recovery crypto | `RecoveryTokens.cpp:37–39` | Crypto initialization/internal dependency failure and final fallback. Bad PIN, envelope and key cases already exercise normal validation. Do not hide the crypto failure solely because libsodium lacks an allocator seam. |
| Rebuild size | `Rebuild.cpp:36` | The private response-count overflow guard requires more than INT32_MAX durable objects. Keep this resource boundary visible until a useful bounded test seam is justified. |
| Placement key space | `CardHierarchy.cpp:83` | A fully widened placement with no endpoints uses integer-spaced keys. Exhaustion requires an impractically large collection. Neighbor exhaustion and widening are tested. |
| Legacy milestone | `CardStore.cpp:807` | An existing end value without a legacy epoch conflicts with earlier front-matter date validation. Keep the defensive guard. |
| Private path invariant | `CardStore.cpp:891` | Callers validate the snapshot path before applying changes. Public bad-path rejection remains tested. |

## Compiler attribution candidates from the initial audit

The remaining records include function-end braces, loop-end braces, aggregate
initializers, a default argument declaration, and a SQL literal. Most are absent
from gcovr's executable-miss list. They need per-record counter evidence before
being annotated; return/aggregate lines must not be ignored merely because nearby
statements execute. In particular:

- `Assets`, `CardReference`, `Git`, `Locations`, `Projects`, `Rebuild`, `Trash`,
  `RecoverySnapshots`, `RecoveryTokens`, and `capi/holder.cpp` include aggregate
  construction/return counters. Check returned fields against existing round-trip
  assertions and inspect their gcov blocks.
- `CardCursor`, `Links`, `Milestones`, `Resources`, `CardHierarchy`, `MilestoneDate`,
  `ProjectRegistry`, and generated conversion endings include closing-brace counters.
- `ProjectMaintenance.cpp:42` and `RuntimeHosts.cpp:111` are lambda attribution;
  callback execution and exception behavior require separate witnesses.
- `CardRepo.cpp:383` is SQL construction attribution; paging/filter tests exercise
  the query, but this does not alone prove that every attached counter is synthetic.
- `CardHistory.h:90` is a default argument declaration.

A block-level spot check with GCC 15.2 `gcov -t -a -b` shows
`Assets.cpp:69` and `:89` each reporting `4*`: two executed blocks with count 2
and one unexecuted block. Thus the aggregate returns execute even though the
baseline lcov trace marks their line records zero. The installed `geninfo` implementation explicitly sets a positive line count to
zero when `geninfo_unexecuted_blocks=1` and this condition holds. Capture now sets
that option to zero and allows the inconsistency warning, preserving the original
gcov count instead of manufacturing a miss. It is not evidence
that the entire return statement should be omitted.

The initial audit left these records visible. The follow-up below records which
closing-brace counters now have enough evidence for a source-only exclusion.
Preserve real safeguards and require a precise witness before changing reporting policy.

## Baseline inventory

| File | Missed lines | Line numbers |
|---|---:|---|
| [core/src/api/ContractCApi.cpp](../src/api/ContractCApi.cpp) | 32 | 226, 257, 265, 274, 282, 292, 300, 308, 316, 326, 334, 343, 351, 360, 370, 385, 393, 406, 418, 426, 1470, 1547, 1555, 1571, 1580, 1591, 1626, 1638, 1657, 1664, 1671, 1677 |
| [core/src/api/HostCApi.cpp](../src/api/HostCApi.cpp) | 24 | 188, 189, 190, 191, 192, 193, 214, 215, 216, 217, 218, 219, 246, 247, 248, 249, 250, 251, 279, 280, 281, 282, 283, 284 |
| [core/src/api/RecoverySnapshots.cpp](../src/api/RecoverySnapshots.cpp) | 14 | 56, 72, 87, 102, 232, 250, 260, 273, 281, 286, 294, 479, 480, 493 |
| [core/src/api/History.cpp](../src/api/History.cpp) | 11 | 52, 64, 117, 123, 145, 146, 147, 194, 198, 277, 313 |
| [core/src/api/Git.cpp](../src/api/Git.cpp) | 8 | 35, 37, 38, 39, 119, 131, 137, 192 |
| [core/src/api/RecoveryTokens.cpp](../src/api/RecoveryTokens.cpp) | 5 | 37, 38, 39, 143, 167 |
| [core/src/api/Cards.cpp](../src/api/Cards.cpp) | 3 | 149, 198, 200 |
| [core/src/api/Links.cpp](../src/api/Links.cpp) | 3 | 73, 95, 249 |
| [core/src/card/CardStore.cpp](../src/card/CardStore.cpp) | 3 | 807, 858, 891 |
| [core/src/api/Assets.cpp](../src/api/Assets.cpp) | 2 | 69, 89 |
| [core/src/api/Projects.cpp](../src/api/Projects.cpp) | 2 | 44, 57 |
| [core/src/api/Rebuild.cpp](../src/api/Rebuild.cpp) | 2 | 34, 36 |
| [core/src/api/Trash.cpp](../src/api/Trash.cpp) | 2 | 27, 36 |
| [core/src/card/CardHierarchy.cpp](../src/card/CardHierarchy.cpp) | 2 | 30, 83 |
| [core/src/api/CardCursor.h](../src/api/CardCursor.h) | 1 | 28 |
| [core/src/api/CardReference.cpp](../src/api/CardReference.cpp) | 1 | 34 |
| [core/src/api/Locations.cpp](../src/api/Locations.cpp) | 1 | 48 |
| [core/src/api/Milestones.cpp](../src/api/Milestones.cpp) | 1 | 39 |
| [core/src/api/ProjectMaintenance.cpp](../src/api/ProjectMaintenance.cpp) | 1 | 42 |
| [core/src/api/Resources.cpp](../src/api/Resources.cpp) | 1 | 70 |
| [core/src/api/RuntimeHosts.cpp](../src/api/RuntimeHosts.cpp) | 1 | 111 |
| [core/src/capi/holder.cpp](../src/capi/holder.cpp) | 1 | 189 |
| [core/src/card/CardRepo.cpp](../src/card/CardRepo.cpp) | 1 | 383 |
| [core/src/history/CardHistory.h](../src/history/CardHistory.h) | 1 | 90 |
| [core/src/model/MilestoneDate.h](../src/model/MilestoneDate.h) | 1 | 129 |
| [core/src/project/ProjectRegistry.cpp](../src/project/ProjectRegistry.cpp) | 1 | 181 |


## Missed lcov records after the initial audit

| File under `src/` | Line records |
|---|---|
| `api/CardCursor.h` | 28 |
| `api/Cards.cpp` | 198, 200 |
| `api/ContractCApi.cpp` | 226, 1470, 1547, 1555, 1571, 1580, 1591, 1626, 1638, 1657, 1664, 1671, 1677 |
| `api/Git.cpp` | 192 |
| `api/History.cpp` | 52, 64, 117, 123, 145, 146, 147, 194, 277 |
| `api/HostCApi.cpp` | 188, 189, 190, 191, 192, 193, 214, 215, 216, 217, 218, 219, 246, 247, 248, 249, 250, 251, 279, 280, 281, 282, 283, 284 |
| `api/Links.cpp` | 73, 95, 249 |
| `api/Milestones.cpp` | 39 |
| `api/ProjectMaintenance.cpp` | 42 |
| `api/Projects.cpp` | 57 |
| `api/Rebuild.cpp` | 36 |
| `api/RecoverySnapshots.cpp` | 56, 72, 87, 102, 232, 250, 260, 273, 281, 286, 294, 479, 493 |
| `api/RecoveryTokens.cpp` | 37, 38, 39 |
| `api/Resources.cpp` | 70 |
| `api/RuntimeHosts.cpp` | 111 |
| `card/CardHierarchy.cpp` | 30, 83 |
| `card/CardRepo.cpp` | 383 |
| `card/CardStore.cpp` | 807, 891 |
| `model/MilestoneDate.h` | 129 |
| `project/ProjectRegistry.cpp` | 181 |

## Follow-up: compiler-generated cleanup counters

The follow-up reviews the 82 records above. GCC 15's JSON and block output identify
29 zero-hit records on otherwise empty closing braces. Their enclosing functions
execute; the disassembly attributes destruction of local C++ values and exception
unwinding to those brace lines. In particular, generated conversion endings call
the result's destructor followed by `_Unwind_Resume`, while ordinary successful
returns have their own executed counters. The loop endings destroy JSON/string
locals on an exception path. No application statement is present on these lines.

Each exclusion is `LCOV_EXCL_LINE` on the reviewed brace alone. Allocation errors,
input validation, explicit catches/throws, return expressions and loop bodies
remain measured. This policy treats compiler-synthesized cleanup attribution as
non-statement coverage; it does not claim that all possible allocation-failure
points have been exercised. The shared allocation-failure boundary tests continue
to run. Other compilers may attribute these blocks differently.

Generated changes live in `contract/generators/core/glue.py`. The generator uses an
exact set of reviewed conversion names, plus the location-configuration map and
milestone-date list. The regression test rejects exclusions on return expressions
or input validation and checks that the only other exclusions are the previously
reviewed 19 output-enum fallback throws.

The raw evidence can be reproduced from `core/` after a coverage run (the source
annotations affect report generation, not GCC's raw counters):

```sh
gcov -j -t -b build-coverage/CMakeFiles/holder.dir/src/api/ContractCApi.cpp.gcno
gcov-dump -l build-coverage/CMakeFiles/holder.dir/src/api/ContractCApi.cpp.gcno
objdump -drl --section=.text --disassemble=_ZN12_GLOBAL__N_1L6from_cERK30holder_storage_location_update build-coverage/CMakeFiles/holder.dir/src/api/ContractCApi.cpp.o
```

### Block evidence and existing test witnesses

Line and block numbers below are from the pre-annotation GCC 15.2 profiles; they
identify the zero-hit record rather than an entire function. Positive function
counts establish execution, with the listed test files supplying behavioral
assertions. For inline headers, use the selected implementation's test object:
`CApiCards_test.cpp.o` for cursor encoding and `CardFrontMatter_test.cpp.o` for
legacy dates. Other copies of those inline functions have zero hits because the
linker selects a single implementation.

| Source | Line → block; enclosing function calls | Behavioral witnesses under `tests/` |
|---|---|---|
| `api/ContractCApi.cpp` | 226 → 99; 6 calls; 1470 → 24; 1 calls; 1547 → 17; 18 calls; 1555 → 35; 17 calls; 1571 → 25; 4 calls; 1580 → 21; 16 calls; 1591 → 36; 8 calls; 1626 → 16; 2 calls; 1638 → 9; 8 calls; 1657 → 9; 12 calls; 1664 → 13; 20 calls; 1671 → 12; 19 calls; 1677 → 8; 7 calls | `CApiLocations_test.cpp`, `CApiCards_test.cpp`, `CApiProjects_test.cpp`, `CApiHistory_test.cpp`, `CApiMaintenance_test.cpp` |
| `api/History.cpp` | 117 → 326; 14 calls; 123 → 461; 14 calls; 277 → 312; 11 calls | `ContractHistory_test.cpp`, `CApiHistory_test.cpp` |
| `api/Links.cpp` | 73 → 38; 19 calls; 95 → 40; 11 calls; 249 → 38; 1 calls | `ContractLinks_test.cpp`, `CApiCards_test.cpp` |
| `api/Milestones.cpp` | 39 → 27; 10 calls | `CApiCards_test.cpp`, `MilestoneUpdate_test.cpp` |
| `api/Projects.cpp` | 57 → 10; 154 calls | `ContractProjects_test.cpp`, `CApiProjects_test.cpp` |
| `api/RecoverySnapshots.cpp` | 250 → 701; 2039 calls; 273 → 892; 2039 calls; 281 → 983; 2039 calls | `ContractRecoverySnapshots_test.cpp` (nested round trip) |
| `api/Resources.cpp` | 70 → 35; 12 calls | `CApiCards_test.cpp`, `CApiMaintenance_test.cpp` (resource paging) |
| `card/CardHierarchy.cpp` | 30 → 27; 127 calls | `CardPlacementResolver_test.cpp` (deleted/tied siblings) |
| `project/ProjectRegistry.cpp` | 181 → 36; 13 calls | `ProjectRegistry_test.cpp` (identity, removals, recovery) |
| `api/CardCursor.h` | 28 → 26; 29 calls | `CApiCards_test.cpp` (pagination and cursor validation) |
| `model/MilestoneDate.h` | 129 → 19; 147 calls | `CardFrontMatter_test.cpp`, `MilestoneUpdate_test.cpp` (legacy dates) |

The following candidates remain measured: `Git.cpp:192`,
`ProjectMaintenance.cpp:42`, `RuntimeHosts.cpp:111`, `CardRepo.cpp:383`, and the
remaining aggregate-construction records in `RecoverySnapshots.cpp`. Those lines
contain application expressions, so this brace-only policy does not apply even
where compiler cleanup contributes to their counters. Dependency failures and
invariant guards also remain measured.

## Missed lcov records after the compiler-cleanup review

| File under `src/` | Line records |
|---|---|
| `api/Cards.cpp` | 198, 200 |
| `api/Git.cpp` | 192 |
| `api/History.cpp` | 52, 64, 145, 146, 147, 194 |
| `api/HostCApi.cpp` | 188, 189, 190, 191, 192, 193, 214, 215, 216, 217, 218, 219, 246, 247, 248, 249, 250, 251, 279, 280, 281, 282, 283, 284 |
| `api/ProjectMaintenance.cpp` | 42 |
| `api/Rebuild.cpp` | 36 |
| `api/RecoverySnapshots.cpp` | 56, 72, 87, 102, 232, 260, 286, 294, 479, 493 |
| `api/RecoveryTokens.cpp` | 37, 38, 39 |
| `api/RuntimeHosts.cpp` | 111 |
| `card/CardHierarchy.cpp` | 83 |
| `card/CardRepo.cpp` | 383 |
| `card/CardStore.cpp` | 807, 891 |

## Follow-up: crypto dependency failures and internal enum fallbacks

`tests/CryptoDependencyFailure_test.cpp` runs in a separate executable on Linux
with GNU/Clang linker wrapping. Only this executable wraps `sodium_init` and
`crypto_aead_xchacha20poly1305_ietf_encrypt`; production libraries and the main
Catch runner keep the ordinary dependency calls. A scoped, thread-local flag
injects one failing return value, then restores pass-through behavior. This is
return-value injection, not an attempt to exhaust memory or use malformed pointers.
It verifies Holder's response to the dependency's failure result; a successful
retry after injection is disabled does not claim that an actually broken crypto
subsystem can recover.

Three test cases (86 assertions across their sections) cover:

- Initialization failure raises `PrivacyCryptoFailed` before returning key material.
- Token export reports `HOLDER_ERROR_DATA_CORRUPTED` on initialization or encryption
  failure and publishes no result. Project key identity, timestamp, key bytes and
  Git HEAD stay unchanged; the existing token is still readable. A healthy export
  works after injection is disabled.
- Existing-project and global token import translate an initialization failure to
  `HOLDER_ERROR_RECOVERY_TOKEN_INVALID`, matching the current recovery policy.
  Neither the existing project/key nor an empty target is changed. Retrying with
  the same valid token and healthy dependency succeeds and restores the same key.

The initialization and encryption failure blocks in `privacy/CryptoService.cpp`
previously had source exclusions. Those exclusions are removed now that the
handlers execute in tests. The export test also reaches `api/RecoveryTokens.cpp:37–38`,
which the previous report showed as missed.

Eight single-line internal fallback annotations accompany this batch:

| Source | Lines | Reachability argument and existing witnesses |
|---|---|---|
| `api/History.cpp` | 52, 64 | Private output enum conversions explicitly handle every `ProjectHistoryObjectKind` / `HistoryAffectedObjectKind` enumerator, including `Unknown`. These functions do not parse caller-provided integers. History activity/filter tests cover the real kinds; the public input converter remains measured. |
| `api/History.cpp` | 194 | The resolver's private output switch handles `Resolved`, `Ambiguous`, and `NotFound`. `CApiHistory_test.cpp` includes real Git abbreviation collisions as well as missing and resolved revisions. |
| `api/RecoverySnapshots.cpp` | 56, 72, 87, 102 | Private serializers handle every declared enum. Persisted strings and imported snapshots pass through separate validating converters. `ContractRecoverySnapshots_test.cpp` tests unknown persisted link/provider/encoding/privacy values and malformed imported snapshots; those rejection paths remain measured. |
| `api/RecoveryTokens.cpp` | 39 | `with_contract_errors` translates six of the eight declared `PrivacyErrorCode` values before this catch. This catch translates the other two (`RecoveryTokenInvalid`, `PrivacyCryptoFailed`). The final rethrow protects a future code; valid current codes cannot reach it. Crypto failure translation now has a real dependency-return-value test. |

No host-registration catch, resource-limit guard, card invariant or initializer
expression is excluded by this batch. No production behavior changes.

## Missed lcov records after the crypto dependency batch

| File under `src/` | Line records |
|---|---|
| `api/Cards.cpp` | 198, 200 |
| `api/Git.cpp` | 192 |
| `api/History.cpp` | 145, 146, 147 |
| `api/HostCApi.cpp` | 188, 189, 190, 191, 192, 193, 214, 215, 216, 217, 218, 219, 246, 247, 248, 249, 250, 251, 279, 280, 281, 282, 283, 284 |
| `api/ProjectMaintenance.cpp` | 42 |
| `api/Rebuild.cpp` | 36 |
| `api/RecoverySnapshots.cpp` | 232, 260, 286, 294, 479, 493 |
| `api/RuntimeHosts.cpp` | 111 |
| `card/CardHierarchy.cpp` | 83 |
| `card/CardRepo.cpp` | 383 |
| `card/CardStore.cpp` | 807, 891 |

## Follow-up: host-registration mutex failure

`tests/HostRegistrationFailure_test.cpp` adds a separate Linux GNU/Clang test
executable using libstdc++. It wraps `pthread_mutex_lock` for this executable only.
The target checks for libstdc++ because its inline `std::mutex::lock` exposes the
pthread call to linker wrapping; it does not assume other standard libraries do. A scoped,
thread-local flag makes the next lock attempt return `EAGAIN` without acquiring
the mutex; all other calls pass through. The test checks that this fault was
consumed during registration and that the resulting `std::system_error` becomes
a `HOLDER_ERROR_RUNTIME` C error with the platform's error message.

The test has time-zone, keyring and storage sections, with 83 assertions in total.
For each service it installs an existing provider, rejects a replacement under
the injected lock failure, then verifies:

- Registration invoked neither provider's service callbacks.
- The rejected provider's callback owner was destroyed exactly once.
- The existing provider remains alive and services the next request.
- With injection disabled, a new registration succeeds, releases the old owner
  once, and services requests through the new provider.
- A further replacement releases the successful new owner once; neither of the
  earlier owners is released a second time.

The callback data uses heap-owned shared state, so a retained provider never
holds a pointer to a test's stack. The isolated runner also avoids changing
registries in the ordinary test process. This checks Holder's handling of a
platform error return, rather than inducing resource exhaustion or modifying
mutex memory. It does not claim that this error commonly occurs with a healthy
platform mutex.

The covered registration paths publish under `std::mutex` in `TimeZones.cpp`,
`PlatformKeyring.cpp` and `RuntimeHosts.cpp`. SSH signer registration has no
corresponding lock, so it is not included. No production source or coverage
exclusions change in this batch.

## Missed lcov records after the registration-lock batch

| File under `src/` | Line records |
|---|---|
| `api/Cards.cpp` | 198, 200 |
| `api/Git.cpp` | 192 |
| `api/History.cpp` | 145, 146, 147 |
| `api/HostCApi.cpp` | 188, 192, 193, 214, 218, 219, 246, 247, 248, 249, 250, 251, 279, 283, 284 |
| `api/ProjectMaintenance.cpp` | 42 |
| `api/Rebuild.cpp` | 36 |
| `api/RecoverySnapshots.cpp` | 232, 260, 286, 294, 479, 493 |
| `api/RuntimeHosts.cpp` | 111 |
| `card/CardHierarchy.cpp` | 83 |
| `card/CardRepo.cpp` | 383 |
| `card/CardStore.cpp` | 807, 891 |

## Follow-up: repository allocation cleanup and parser resource errors

`tests/ReadAllocationFailure_test.cpp` adds an isolated runner on 64-bit Linux
with GNU/Clang and the GNU C++11 string ABI. The configuration checks the string
ABI before enabling the YAML symbol wrapper. Production libraries use their
ordinary allocation and YAML functions.

The repository test sweeps one failed allocation at a time through
`CardRepo::list_collection_page`, covering both an unfiltered page and an indexed
tag page. It tests the C++ allocations exposed by linker wrapping of `operator new`
in the statically linked code; SQLite's allocator and shared-library internals
are not injected. Each failure must propagate `std::bad_alloc`, publish no vector,
and allow a complete retry. The tests assert unchanged SQLite write counts,
unchanged prepared-statement counts relative to the connection's pre-read
baseline, and unchanged IDs/titles. They stop when a call completes without
consuming the next failure, with an explicit iteration cap.

The parser regression injects `std::bad_alloc` at the `YAML::Load(std::string)`
boundary. Previously the broad `std::exception` handler treated it as malformed
front matter and returned the complete raw file as body content, exposing the
front matter and dropping parsed identity/links. `CardFrontMatter.cpp` now
rethrows `std::bad_alloc` before the malformed-input fallback. A healthy retry
parses the original metadata and body completely, and malformed YAML still uses
the original fallback. This change concerns a propagated resource exception; it
does not promise that every internal dependency failure retains that exception
type through the dependency's own converters.

These two test cases have 235 assertions across their sections. The repository
sweep reaches the previously zero-hit `CardRepo.cpp:383` record without an
exclusion. No new source exclusions are introduced in this batch.

### Dependency limitation found during investigation

A whole-call allocation sweep through the JSON-based C read APIs is not a valid
uniform recoverability test with the installed nlohmann-json 3.11.3. The debugger
traces an abort to `json_value::destroy`: it allocates a vector to flatten nested
arrays/objects, and the allocation exception crosses a `noexcept` destructor.
The process terminates before Holder's C ABI guard can translate the error.
The local implementation is `/usr/include/nlohmann/json.hpp`, especially the
flattening stack in `json_value::destroy` around lines 567–613 and the destructor
at line 1257. The triggering read was constructing the card-pagination scope.

The test suite therefore targets the Holder-owned repository path and the YAML
exception boundary explicitly. It does not skip arbitrary allocation positions,
change dependency destructors or system headers, or claim all process-wide
allocation failures are recoverable. JSON aggregate cleanup counters in the
remaining inventory still need separate review. This dependency limitation
remains unresolved by the parser fix.

## Missed lcov records before the final audit

| File under `src/` | Line records |
|---|---|
| `api/Cards.cpp` | 198, 200 |
| `api/Git.cpp` | 192 |
| `api/History.cpp` | 145, 146, 147 |
| `api/HostCApi.cpp` | 188, 192, 193, 214, 218, 219, 246, 247, 248, 249, 250, 251, 279, 283, 284 |
| `api/ProjectMaintenance.cpp` | 42 |
| `api/Rebuild.cpp` | 36 |
| `api/RecoverySnapshots.cpp` | 232, 260, 286, 294, 479, 493 |
| `api/RuntimeHosts.cpp` | 111 |
| `card/CardHierarchy.cpp` | 83 |
| `card/CardStore.cpp` | 807, 891 |

## Final audit of the remaining 0.2%

| Classification | Original records | Final treatment |
|---|---:|---|
| Rebuild response overflow | 1 | New adapter boundary tests, no exclusion |
| Covered lambda entry attribution | 1 | Exact file/symbol normalization from its positive function-entry count |
| Compiler expression/cleanup attribution | 8 | Single-line annotations, with executed-body and round-trip witnesses |
| Guards behind earlier validation/error absorption | 7 | Single-line defensive annotations; public rejection paths remain measured |
| Host registration defensive handlers | 15 | Generated single-line annotations; allocation and mutex failure handlers remain measured |
| Full-collection numeric resource boundary | 1 | Explicitly documented resource-limit exclusion; guard retained |

### Rebuild boundary test

The isolated read-allocation runner now wraps `Rebuilder::rebuild_project` at its
backend/adapter boundary. For each of `cards`, `ai_messages`, `ai_threads` and
`links`, it returns `INT32_MAX` and `INT32_MAX + 1` without allocating that many
records. The API must preserve the largest valid count exactly, or return
`HOLDER_ERROR_REBUILD_FAILED` with `rebuild count exceeds API range`, a null result,
and no database writes for the overflow. A pass-through call also performs a real
empty-project rebuild. The runner now has 285 assertions across three test cases.
This tests the actual private conversion guard and its error propagation rather
than substituting the API's error handler.

### Compiler attribution evidence

GCC 15's JSON identifies a zero-hit counter at the following block in an otherwise
executed function. Normal expression evaluation is attributed to adjacent lines
or discriminators; debug disassembly and existing round trips demonstrate the
ordinary path. For example, snapshot enum-name calls execute in normal code while
GCC associates their exception continuation with the initializer's source record.
The retained field converters and validating parsers remain measured.

| Source / original line | Zero-hit block | Enclosing function calls before final batch | Witness |
|---|---:|---:|---|
| `api/Git.cpp:192` | 75 | 5 | Scheduled sync tests in `ContractGit_test.cpp` / C API sync tests |
| `api/RecoverySnapshots.cpp:232` | 601 | 2039 | Nested recovery snapshot round trip, including link target types |
| `api/RecoverySnapshots.cpp:260` | 747 | 2039 | Recovery placement encoding and asset round trip |
| `api/RecoverySnapshots.cpp:286` | 1029 | 2039 | Recovery storage-provider round trip |
| `api/RecoverySnapshots.cpp:294` | 1187 | 2039 | Plain and encrypted recovery privacy modes |
| `api/RecoverySnapshots.cpp:479` | 32 | 4 | Location configuration/identity reconstruction assertions |
| `api/RecoverySnapshots.cpp:493` | 128 | 4 | Placement location, encoding, size and digest reconstruction assertions |
| `api/RuntimeHosts.cpp:111` | 20 | 7 | Keyring lookup success/failure and host lifetime tests; zero block belongs to cleanup/unwind |

`api/ProjectMaintenance.cpp:42`, block 73 with eight lambda calls, gets a reporting
correction instead of an annotation. The exact mangled lambda symbol is allowlisted
in `scripts/coverage_filters.py`. A positive matching function-entry count in the
same file is mandatory; only its zero entry-line counter and the line-hit summary
can change. Positive counters, every function record and other missed lines stay
intact. Existing hierarchy-repair tests exercise planning and application.
The raw JSON/disassembly investigation is in
`build-coverage/final-counter-evidence.json` (a local, regenerable artifact).

### Defensive invariants

| Source / original lines | Why valid current call paths cannot reach the fallback |
|---|---|
| `api/Cards.cpp:198,200` | `placement_intent` validates every incoming intent before the private placement translator. `ContractContext_test.cpp` tests invalid C++ intents and the generated C API rejects invalid integer choices. |
| `api/History.cpp:145–147` | Current encrypted metadata reads occur only inside optional display enrichment, whose helpers catch privacy failures and preserve path-only history. Project lookup, Git reading, filtering and response sizing do not emit `PrivacyError`. Actual card-history key errors remain measured and tested. |
| `card/CardStore.cpp:807` | Parsed milestone dates require exactly one validated date or datetime, so an existing valid end date always has an epoch. Invalid front matter and mixed legacy/new updates fail earlier. Legacy milestone and malformed-date tests retain those checks. |
| `card/CardStore.cpp:891` | Every caller passes copied source snapshots validated by move, trash, restore, history restore or `placement_change`. A later database edit cannot mutate the copied value. Existing damaged-source/path tests exercise the public validation, which remains measured. |

### Host registration handlers

The generator annotates exactly 15 records: the four post-registration
`invalid_argument` returns, four unknown-error returns, four terminal cleanup
brace records, and the SSH-only generic exception catch/return and following
catch-dispatch line. Callback validation, input enum/string/span validation,
allocation errors and the three real mutex-system-error handlers stay measured.

`RuntimeHosts.cpp` registers callback adapters without invoking their methods.
The implemented operations allocate/move strings, vectors and shared pointers,
and (except SSH) publish under a mutex. Standard allocation and mutex errors
already have tests. Input validation prevents the internal storage-kind
`invalid_argument`. No current operation emits a non-standard exception.
SSH's generic standard-exception fallback protects future adapters and string/
vector length errors requiring unrepresentably large, otherwise valid input
objects. Its actual allocation handler remains measured. Exceptions from a host
cleanup destructor cannot be safely used here: that destructor is `noexcept`.

`contract/generators/core/host_glue.py` emits the annotations. The generator
regression verifies their exact count and checks that all validation, ownership
acquisition, allocation handling and non-SSH mutex-error handlers stay measured.
No host callback invocation/error path is excluded.

### Explicit resource-boundary exclusion

`card/CardHierarchy.cpp:83` protects the final fully widened sibling window. With
both endpoints open, its sort-key construction spaces values from 1 through the
collection size; failure requires a collection beyond reliable distinct-double
spacing. Constructing such a collection is outside a practical in-memory test
fixture. The single throw is classified as a resource/numeric safeguard, not as a
compiler artifact or a claim of mathematical unreachability. Neighbor exhaustion,
tied keys, widening and ordinary full-window re-spacing stay measured in the
placement tests. The executable still throws if the safeguard ever triggers.

## Current missed records

None within the documented measurement scope. The JSON dependency limitation and
resource-boundary exclusion above remain explicit limitations of that scope.

## PR #426 portability verification

Windows and macOS CI exposed three platform assumptions after the Linux audit:

- The journal short-write fixture compared path spellings. libgit2 can expand a
  Windows short path or resolve macOS `/var` to `/private/var`, so the intended
  failure was never injected. It now matches the filename plus filesystem identity
  of the existing parent directory. The fixture deliberately uses a `.` alias and
  asserts exactly one truncated write, reproducing the issue on Linux as well.
- The blocked-checkout test assumed LF output regardless of Git configuration.
  It now tests repository-local `core.autocrlf=false` and `true`, retaining exact
  byte comparisons for preserved local work and checking the configured LF/CRLF
  output after successful checkout.
- The privacy safety scan rejected a CRLF `HolderPriv1` header even though the
  envelope decoder accepts CRLF. Working-tree and index checks now accept either
  LF or CRLF while continuing to reject altered magic. The regression encrypts a
  real envelope, verifies decryption of both forms, and checks cards, resources
  and locations in both the working tree and an index configured to preserve
  their bytes with `-text` attributes. Modified header prefixes are still rejected.

The focused regression selection passes 272 assertions across eight test cases.
The complete Linux suite passes 890 CTest entries and retains 100% measured line
and function coverage. Logs: `build-coverage/ci-426-targeted-tests.log` and
`build-coverage/ci-426-verification.log`. Platform CI will verify the same changes
on Windows and macOS. No coverage exclusions were added for these fixes.
