# Coding-agent instructions

# Icons

Don't add the `androidx.compose.material:material-icons-extended` dependency to get a single missing Material icon. It pulls in the whole icon set unconditionally — `material-icons-extended`'s `classes.jar` is ~36MB / 11,105 compiled classes, vs `material-icons-core`'s ~850KB / 293 — and this project has no `minifyEnabled`/R8 shrinking configured for release builds, so none of that gets tree-shaken back out; nearly all of it ships. This has already slipped in and been removed twice (commits `b4ab295`, `a05e6c9`) — check `app/build.gradle.kts` for `material.icons.extended` before assuming it's still absent.

Instead, copy the needed glyph's exact path data into a new `res/drawable/ic_<name>.xml` vector drawable (24dp viewport, single `<path android:fillColor="#FF000000" android:pathData="..."/>`), and reference it via `Icon(painterResource(R.drawable.ic_<name>), contentDescription = "...")` — `Icon()` tints it automatically via `LocalContentColor`, so the hardcoded fill color doesn't matter. See `ic_undo.xml`, `ic_redo.xml`, `ic_fullscreen.xml`, `ic_account_tree.xml`, `ic_folder.xml`, `ic_link.xml` for existing examples.

To get exact path data without hand-tracing an SVG: write a throwaway JVM unit test under `app/src/test` that imports the icon from wherever it currently resolves, walks `ImageVector.root` recursively collecting each `VectorPath.pathData: List<PathNode>`, and renders each `PathNode` variant to its SVG command-letter equivalent (M/L/H/V/C/S/Q/T/A/Z, lowercase for the Relative* variants) — vector drawable `pathData` uses identical syntax, so the output pastes straight in. Run via `./gradlew :app:testDebugUnitTest --tests "..."`, write the result to a file (Gradle doesn't surface test stdout directly), then delete the test.

# Core dependency ownership

Make holder-core changes in the standalone workspace repository at `../holder-core`
and test them there first. Do not edit embedded dependency copies inside holder-android.

Use the published SDK or package through shared core selection tooling when the
Android build supports it. The current checked-in Gradle and CMake configuration
still builds embedded core source; do not assume the daemon's SDK configuration
works for Android or introduce a build migration as part of an unrelated task.
Follow the current Android build configuration and test the consumer against the
exact core version needed by the task.

Legacy embedded checkouts and their Git pointers are not the normal shared
dependency-update workflow. Do not automatically advance or reset them after
switching branches, and do not commit existing checkout drift as an unrelated
change. Changes to dependency selection or source integration must be part of
the task's explicit scope.

# XML comments

Never use `--` inside an XML comment (`<!-- ... -->`), including as an em dash or separator. It breaks the manifest merge with a vague `ManifestMerger2$MergeFailureException` that gives no hint the punctuation is the cause — this has already bitten a comment written for this exact reason more than once. Use `;` or `,` instead wherever you'd reach for `--`.

# Native build cache

If `./gradlew :app:assembleDebug` fails on one ABI (typically `x86_64`) with a missing header from a vcpkg-provided library (e.g. `sqlite3.h`), don't work around it by restricting the build to a single ABI (`-Pholder.android.abis=arm64-v8a`) and moving on. It's almost certainly a stale CMake configure cache under `app/.cxx/Debug/*/<abi>` left over from an earlier interrupted or partial build — AGP treats that cache as "already configured" and skips re-running vcpkg's manifest install for that triplet, so the triplet's `build/vcpkg_installed/<triplet>/` directory never gets created at all. Fix: `rm -rf app/.cxx/Debug/*/<abi>` and rebuild, which forces a fresh configure and lets vcpkg actually install the manifest. Only if that doesn't fix it is it worth investigating further — don't reach for an ABI restriction as the first move.
