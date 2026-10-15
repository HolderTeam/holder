# holderctl

The command line client for the Holder daemon. `holderctl` talks to a running `holderd` over its HTTP API,
and `holderctl ensure` makes sure a compatible daemon is running (see
[`daemon/docs/ensure.md`](../daemon/docs/ensure.md)); client applications call that rather than starting a
daemon themselves.

It is also the reference client. Commands for working with Holder use only the documented HTTP contract
([`api/openapi.yaml`](../api/openapi.yaml) and [`api/docs/CLIENTS.md`](../api/docs/CLIENTS.md)), so its
source shows how any client can do the same. Only the lifecycle and admin commands (`ensure`, `start`,
`restart`, `database`) manage the daemon directly.

## Card hierarchy

```sh
holderctl move <card> --into <card>    # append as a child
holderctl move <card> --before <card>  # or --after <card>
holderctl move <card> --top-level      # append at the project's top level
holderctl trash <card>                 # its children move up a level
holderctl trash <card> --subtree       # its live descendants go to Trash too
holderctl repair --dry-run             # list cards hidden under missing or trashed parents
holderctl repair                       # make them reachable again
```

Cards are named by ID, an unambiguous ID prefix or an exact title. Placement, trash and repair rules live in
Holder Core; holderctl sends the request and reports the result.

## Layout

- `src/holderctl.cpp` - the entry point and command dispatch.
- `src/commands/` - one file per group of commands (`card`, `project`, `sync`, `ensure`, ...).

## Building

`holderctl` is built together with the daemon, by the CMake project in [`daemon/`](../daemon/), because it
links the same Holder Core library and shares its version:

```sh
cd daemon
./make.sh build Debug     # builds holderd and holderctl into daemon/build
```

The command tests live with the daemon's tests in `daemon/tests/` (`HolderCtl*_test.cpp`,
`MilestoneDateTime_test.cpp`), because they compile these sources and run against a real `holderd`.

## Shared code

The few platform files that `holderd` and `holderctl` both need are in [`common/`](../common/).
