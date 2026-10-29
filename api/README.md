# api

The HTTP contract between the Holder daemon and its clients.

- [`openapi.yaml`](openapi.yaml) - the OpenAPI description of every endpoint. It is the canonical
  contract: a change to the daemon's HTTP behaviour changes this file in the same pull request.
  The daemon serves it at `GET /openapi.yaml`, and the Python client in [`python/holder/`](../python/holder/)
  is generated from a tested snapshot of it. The document uses OpenAPI 3.2. It is generated, so
  never edit it by hand: see [Changing the contract](#changing-the-contract).
- [`openapi.json`](openapi.json) - the same document as JSON, generated with it. The daemon
  compiles it in and serves it at `GET /openapi.json`, so a daemon always serves the contract it
  was built from.
- [`openapi-base.json`](openapi-base.json) - the API metadata, servers and security shared by
  every route.
- [`API_VERSION`](API_VERSION) - the compatibility version clients check (`MAJOR.MINOR`). It is
  separate from the release version in the root [`VERSION`](../VERSION). A client states the range
  it supports with `holderctl ensure --api-min` and `--api-max-exclusive`.
- [`docs/CLIENTS.md`](docs/CLIENTS.md) - how a client finds, authenticates to and uses a daemon.
- [`docs/event-streams.md`](docs/event-streams.md) - the server-sent event streams and how to
  stay in sync with them.

How the daemon starts and stops (`holderctl ensure`, the idle exit) is documented with the
daemon, in [`daemon/docs/`](../daemon/docs/).

## Changing the contract

Every endpoint is declared in Holder's Python [`contract/`](../contract/). Run
`make -C contract update` after changing a route, model or API metadata; it generates
`openapi.yaml`, `openapi.json` and the contract's C++ files. This includes the AI dispatcher and
JSON codec in `daemon/src/api/generated/AiRoutes.h` and `AiJson.h`. The contract test checks that
the generated dispatcher serves the Python declarations' AI route inventory.
`Contract / Check` verifies the generated files are current.

Holder's own clients (the desktop app, `holderctl`, the Python client) are clients like any
other: they use only what this contract documents, never undocumented behaviour or the daemon's
or core's internals. Third-party clients rely on the same document.

- **Until Holder 1.0, `API_VERSION` stays at 0.1.** Holder promises no stable API before 1.0, and
  the contract changes knowingly as it moves to [`contract/`](../contract/). A pull request that
  breaks the contract updates Holder's own clients in the same pull request and is labelled
  `api: unreleased change`, which the `API / Contract` workflow accepts in place of a bump.
- **From Holder 1.0, breaking changes need a new `API_VERSION`.** Clients accept a range of
  `API_VERSION`s, so a change that would break an existing client must move `API_VERSION` out of
  the range they accept: the next major version. The `API / Contract` workflow checks every pull
  request that changes this directory with [oasdiff](https://github.com/oasdiff/oasdiff) and fails
  on a breaking change without that bump or the label. `API_VERSION` never goes down.
- **Changes to something not yet released** (for example an endpoint added since the last release)
  break no one. A maintainer can accept them without a bump by labelling the pull request
  `api: unreleased change`.
- **Changes in meaning** that a schema comparison cannot see, such as a field that now accepts
  `null` with a new effect or a default that changes, must be written into the declarations'
  descriptions in the same pull request, and treated as breaking if an existing client could
  misbehave.

## Debian source package

A Debian source package is built from `daemon/` alone. Its workflow copies this directory into
`daemon/` first, and the CMake build uses that copy when the directory beside `daemon/` is not
there. The copy is ignored by Git (`daemon/.gitignore`).
