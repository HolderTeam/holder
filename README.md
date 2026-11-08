# Holder

**Your knowledge belongs to you.**

Holder turns your knowledge into something you can own, inspect, automate and build upon.

The Holder Framework is a complete, local-first knowledge system: a set of open tools for storing, organising, connecting and working with your information without surrendering control of it to somebody else's platform.

It provides the services and interfaces that make Holder available to applications, scripts and people across different operating systems. Desktop clients, command-line tools and other software can all work with the same local knowledge through a stable OpenAPI interface, while your data remains in formats and repositories you control.

Holder Core provides the underlying algorithms, data structures and formats. Holder Framework builds on that foundation to provide the running system around them: the Holder daemon, `holderctl`, application lifecycle support, packaging and the machinery used to build and release Holder across platforms.

The goal is not simply to make another notes application. It is to provide a durable personal computing foundation for knowledge: open, interoperable, automatable and yours.

## Getting Holder

Download Holder for Windows, macOS or Linux from [holder.team](https://holder.team/), or from the [releases page](https://github.com/HolderTeam/holder/releases).

## In this repository

- [`contract/`](contract/) - Holder's public programming interface, declared in Python using Pydantic and FastAPI. It generates the C API, OpenAPI and code that connects implementations and clients.
- [`core/`](core/) - Holder Core (`libholder`): the engine, its C API and its own CMake build, which does not depend on anything else here.
- [`daemon/`](daemon/) - the Holder daemon (`holderd`), its HTTP API and its CMake build, which also builds `holderctl`.
- [`api/`](api/) - the generated HTTP schema (`openapi.yaml`), `API_VERSION` and client documentation.
- [`cli/`](cli/) - `holderctl`, the command line client, including `holderctl ensure`.
- [`common/`](common/) - platform code shared by the daemon and `holderctl`.
- [`apps/desktop/gtk/`](apps/desktop/gtk/) - the Holder desktop app (GTK 4 and libadwaita, built with Meson), a client of the daemon's HTTP API on Linux, macOS and Windows.
- [`apps/mobile/android/`](apps/mobile/android/) - the Holder Android app (Kotlin and Compose), which embeds core through its C API and works without a daemon.
- [`python/`](python/) - Python libraries: [`holder/`](python/holder/), Holder for Python scripts that work with a running Holder installation, and [`kit/`](python/kit/), Holder Kit, which embeds core for offline work.
- [`release/`](release/) - candidate assembly and checks in [`release/staging/`](release/staging/), signing, verification and publishing of releases, and the website ([`release/website/`](release/website/), www.holder.team), including its developer guides.

## Developing Holder

Holder uses contract-first development. Public operations, data types and their meaning are
defined in the [Python contract](contract/README.md). The C API and OpenAPI specification are
generated from it; core and the daemon implement the behavior behind those interfaces. Start
an interface change in the contract, regenerate, and update implementations and affected clients
together.

For component-specific work, start with its guide:

- [Core](core/README.md#consumption-model) and [daemon](daemon/docs/development.md), including [core build options](daemon/docs/core-builds.md)
- [HTTP contract](api/README.md#changing-the-contract) and [GTK desktop](apps/desktop/gtk/README.md#development)
- [Android](apps/mobile/android/README.md#local-development) and its [development notes](apps/mobile/android/DEVELOPMENT.md)
- [Holder for Python](python/holder/docs/development.md) and [Holder Kit](python/kit/docs/development.md)

## Version

Everything in this repository (core, the daemon, `holderctl`, the desktop app, Holder Kit, the
Python client, and the staging and release tooling) has one version: the [`VERSION`](VERSION) file
at the root. Native builds and both Python packages read it directly in a full checkout.
Python source distributions include generated version/build metadata automatically.
For other source exports, prepare the local metadata using
`release/scripts/version.py --prepare-source <core|daemon|desktop|kit|holder-python> --destination <source-directory>`;
the Launchpad archive generator includes it automatically for core and Kit.
Change the version with `release/scripts/version.py <version>`, which updates only
the root file. `Repository / Checks` checks version derivation and source exports. The daemon's
[`API_VERSION`](api/API_VERSION) is separate: it versions the HTTP contract that clients check, not
the release. Core's library SONAME is separate too.

Android's package-version metadata is still independent while
[#130](https://github.com/HolderTeam/holder/issues/130) is completed.

## Licence

Holder Framework is free software, licensed under the GNU General Public License, version 3. See [LICENSE](LICENSE).
