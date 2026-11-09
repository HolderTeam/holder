# Core coverage audit

Baseline: 15,010 / 15,135 lines (99.2%) and 1,881 / 1,881 functions (100%).
The table below records the 125 missed lcov records before this audit. Line numbers
refer to that baseline; generated output may move when regenerated. gcovr classified
72 of these records as executable. An unhit source record alone does not establish
an untested behavior, and a difficult failure path is not automatically an artifact.

## Latest result

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

## Current missed lcov records

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
