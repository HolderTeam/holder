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
- [`rig/`](rig/) - how the contract is generated from the daemon's C++ declarations with
  [RIG](https://github.com/zeth/rig).
- [`API_VERSION`](API_VERSION) - the compatibility version clients check (`MAJOR.MINOR`). It is
  separate from the release version in the root [`VERSION`](../VERSION). A client states the range
  it supports with `holderctl ensure --api-min` and `--api-max-exclusive`.
- [`docs/CLIENTS.md`](docs/CLIENTS.md) - how a client finds, authenticates to and uses a daemon.
- [`docs/event-streams.md`](docs/event-streams.md) - the server-sent event streams and how to
  stay in sync with them.

How the daemon starts and stops (`holderctl ensure`, the idle exit) is documented with the
daemon, in [`daemon/docs/`](../daemon/docs/).

## Changing the contract

Endpoints are moving to Holder's contract, [`contract/`](../contract/), which declares them once in
Python and generates the daemon's routes for them ([`contract/HTTP.md`](../contract/HTTP.md)). The
rest are declared in C++ in [`daemon/src/api/`](../daemon/src/api/): route, parameters, request and
response types, errors and descriptions. [RIG](rig/) reflects those declarations and generates their
document, `rig/openapi.rig.json`, together with the daemon's request dispatcher and JSON code for
them. `make -C contract update` merges the contract's endpoints into RIG's document to write
`openapi.yaml` and `openapi.json`.

To change an endpoint the contract serves, change the contract and run `make -C contract update`.
To change any other, change its declaration and run `make -C api/rig update`, which needs GCC 16 and
the RIG version in [`rig/requirements.txt`](rig/requirements.txt) ([`rig/README.md`](rig/README.md)
has the details), then `make -C contract update`. `Daemon / CI` fails when RIG's document or its
generated code differs from what the declarations produce, and `Contract / Check` when
`openapi.yaml`, `openapi.json` or the contract's generated code is out of date.

Holder's own clients (the desktop app, `holderctl`, the Python client) are clients like any
other: they use only what this contract documents, never undocumented behaviour or the daemon's
or core's internals. Third-party clients rely on the same document.

- **Breaking changes need a new `API_VERSION`.** Clients accept a range of `API_VERSION`s, so a
  change that would break an existing client must move `API_VERSION` out of the range they accept:
  the next minor version while the major version is 0 (0.1 to 0.2), and the next major version from
  1.0. The `API / Contract` workflow checks every pull request that changes this directory with
  [oasdiff](https://github.com/oasdiff/oasdiff) and fails on a breaking change without that bump.
  `API_VERSION` never goes down.
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
