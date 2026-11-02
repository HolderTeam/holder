# Holder Framework

**Your knowledge belongs to you.**

Holder Framework turns your knowledge into something you can own, inspect, automate and build upon.

Holder Framework is a complete, local-first knowledge system: a set of open tools for storing, organising, connecting and working with your information without surrendering control of it to somebody else's platform.

It provides the services and interfaces that make Holder available to applications, scripts and people across different operating systems. Desktop clients, command-line tools and other software can all work with the same local knowledge through a stable OpenAPI interface, while your data remains in formats and repositories you control.

Holder Core provides the underlying algorithms, data structures and formats. Holder Framework builds on that foundation to provide the running system around them: the Holder daemon, `holderctl`, application lifecycle support, packaging and the machinery used to build and release Holder across platforms.

The goal is not simply to make another notes application. It is to provide a durable personal computing foundation for knowledge: open, interoperable, automatable and yours.

## Getting Holder

Download Holder for Windows, macOS or Linux from [holder.team](https://holder.team/), or from the [releases page](https://github.com/HolderTeam/holder/releases).

## In this repository

- [`core/`](core/) - Holder Core (`libholder`): the engine, its C API and its own CMake build, which does not depend on anything else here.
- [`daemon/`](daemon/) - the Holder daemon (`holderd`), its HTTP API and its CMake build, which also builds `holderctl`.
- [`contract/`](contract/) - the Python source of truth for shared operations and types, with generators for core's C API, Android's Kotlin/JNI client, the daemon's HTTP routes and OpenAPI.
- [`api/`](api/) - the generated HTTP schema (`openapi.yaml`), `API_VERSION` and client documentation.
- [`cli/`](cli/) - `holderctl`, the command line client, including `holderctl ensure`.
- [`common/`](common/) - platform code shared by the daemon and `holderctl`.
- [`staging/`](staging/) - assembles and checks build artifacts for each platform.
- [`apps/desktop/gtk/`](apps/desktop/gtk/) - the Holder desktop app (GTK 4 and libadwaita, built with Meson), a client of the daemon's HTTP API on Linux, macOS and Windows.
- [`apps/mobile/android/`](apps/mobile/android/) - the Holder Android app (Kotlin and Compose), which embeds core through its C API and works without a daemon.
- [`python/`](python/) - Python libraries: [`holder/`](python/holder/), Holder for Python scripts that work with a running Holder installation, and [`kit/`](python/kit/), Holder Kit, which embeds core for offline work.
- [`release/`](release/) - signing, verification and publishing of releases, and the website ([`release/website/`](release/website/), www.holder.team), including its developer guides.

## Developing Holder

Start with the guide for the component you are changing:

- [Core](core/README.md#consumption-model) and [daemon](daemon/docs/development.md), including [core build options](daemon/docs/core-builds.md)
- [HTTP contract](api/README.md#changing-the-contract) and [GTK desktop](apps/desktop/gtk/README.md#development)
- [Android](apps/mobile/android/README.md#local-development) and its [development notes](apps/mobile/android/DEVELOPMENT.md)
- [Holder for Python](python/holder/docs/development.md) and [Holder Kit](python/kit/docs/development.md)

These guides describe the current interfaces and build boundaries. When changing an
interface, update its owning contract and tests in the same pull request.

## Version

Everything in this repository (core, the daemon, `holderctl`, the desktop app, Holder Kit, the
Python client, and the staging and release tooling) has one version: the [`VERSION`](VERSION) file
at the root. Parts that are built on their own from their own directory also carry it there, because
their source package holds only that directory: [`core/VERSION`](core/VERSION),
[`apps/desktop/gtk/VERSION`](apps/desktop/gtk/VERSION) and the Python packages' `pyproject.toml`.
Change the version with `release/scripts/version.py <version>`, which sets every copy;
`Repository / Checks` fails a pull request in which they differ. The daemon's
[`API_VERSION`](api/API_VERSION) is separate: it versions the HTTP contract that clients check, not
the release. Core's library SONAME is separate too.

## Licence

Holder Framework is free software, licensed under the GNU General Public License, version 3. See [LICENSE](LICENSE).
