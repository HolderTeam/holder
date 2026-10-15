# Android development notes

## Material icons

Do not add `androidx.compose.material:material-icons-extended` for a missing
icon. It loads the entire icon set: its `classes.jar` is about 36 MB and 11,105
classes, versus about 850 KB and 293 classes for `material-icons-core`. The
release build does not enable R8 shrinking, so those classes ship in the app.
This dependency has been added and removed twice (commits `b4ab295` and
`a05e6c9`); check `app/build.gradle.kts` before changing dependencies.

Use the needed glyph's exact path data in an
`app/src/main/res/drawable/ic_<name>.xml` vector drawable with 24 dp dimensions,
a 24 by 24 viewport and a single black path. Reference it with
`Icon(painterResource(R.drawable.ic_<name>), contentDescription = "...")`;
`Icon()` supplies the content tint. Existing examples include `ic_undo.xml`,
`ic_redo.xml`, `ic_fullscreen.xml`, `ic_account_tree.xml`, `ic_folder.xml` and
`ic_link.xml`.

For path data, a temporary JVM test under `app/src/test` can import the icon,
walk `ImageVector.root` recursively and convert each `VectorPath.pathData`
`PathNode` to its SVG command (`M/L/H/V/C/S/Q/T/A/Z`, lowercase for relative
nodes). Vector drawable `pathData` uses the same syntax. Run the test with
`./gradlew :app:testDebugUnitTest --tests "..."`, write its result to a file
because Gradle does not show test stdout directly, then remove the temporary
test.

## Native build cache

If `./gradlew :app:assembleDebug` reports a missing vcpkg library header such
as `sqlite3.h` for one ABI, inspect the native build cache before narrowing
the target ABIs. An interrupted configure may leave
`app/.cxx/Debug/*/<abi>` marked configured without installing that triplet's
dependencies into `build/vcpkg_installed/<triplet>/`. Remove only the affected
ABI's cache directory and rebuild to force configuration again. If that does
not resolve it, investigate the remaining build failure.

## XML comments

XML comments cannot contain `--`. This includes punctuation used as a dash or
separator; the manifest merger reports an unhelpful `ManifestMerger2` failure.
Use a comma or semicolon inside comments instead.
