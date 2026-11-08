// One product version for the application and its same-checkout native core.
val versionFile = rootProject.layout.projectDirectory.file("../../../VERSION")
val productVersion = providers.fileContents(versionFile).asText.orNull?.trim()
    ?: throw GradleException("Missing repository-root VERSION: ${versionFile.asFile}")
val parts = Regex("([0-9]+)\\.([0-9]+)\\.([0-9]+)").matchEntire(productVersion)
    ?.groupValues?.drop(1)?.map {
        it.toIntOrNull() ?: throw GradleException("Root VERSION component is too large: $productVersion")
    }
    ?: throw GradleException("Root VERSION must be numeric major.minor.patch: $productVersion")
val major = parts[0]
val minor = parts[1]
val patch = parts[2]
if (major !in 0..2099 || minor !in 0..999 || patch !in 0..999) {
    throw GradleException("Android VERSION components must fit major 0..2099, minor/patch 0..999")
}
// Bound components so numeric release ordering stays monotonic and codes stay
// below Google Play's 2,100,000,000 limit. No separate release counter to edit.
val productVersionCode = major * 1_000_000 + minor * 1_000 + patch
if (productVersionCode == 0) {
    throw GradleException("Android VERSION must produce a positive versionCode")
}
extra["holderProductVersion"] = productVersion
extra["holderProductVersionCode"] = productVersionCode
