// On CI, read Maven Central through Google's official mirror first. GitHub's shared runners
// are sometimes refused by Maven Central itself (HTTP 429). An artifact the mirror doesn't have
// yet comes from mavenCentral() after it. Local builds use Maven Central directly.
fun RepositoryHandler.mavenCentralMirrorOnCi() {
    if (System.getenv("CI") == "true") {
        maven("https://maven-central.storage-download.googleapis.com/maven2/") {
            name = "MavenCentralGoogleMirror"
        }
    }
}

pluginManagement {
    repositories {
        google {
            content {
                includeGroupByRegex("com\\.android.*")
                includeGroupByRegex("com\\.google.*")
                includeGroupByRegex("androidx.*")
            }
        }
        // pluginManagement is compiled on its own, so mavenCentralMirrorOnCi() below is out of reach.
        if (System.getenv("CI") == "true") {
            maven("https://maven-central.storage-download.googleapis.com/maven2/") {
                name = "MavenCentralGoogleMirror"
            }
        }
        mavenCentral()
        gradlePluginPortal()
    }
}
plugins {
    id("org.gradle.toolchains.foojay-resolver-convention") version "1.0.0"
}
dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
    repositories {
        google()
        mavenCentralMirrorOnCi()
        mavenCentral()
    }
}

rootProject.name = "Holder"
include(":app")
