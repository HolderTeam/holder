# RIG adoption in Holder

This branch develops RIG metadata in the daemon while `api/openapi.yaml` remains
the checked-in control contract. Normal daemon builds use C++20 and do not need
RIG, Python, or a reflection-capable compiler.

## Control inventory

The OpenAPI 3.1 control currently has 89 paths, 126 operations, and 189 named
schemas. Its operations use 170 declared parameters (89 path, 78 query, and 3
header) and 46 request bodies. Responses include JSON, event streams, plain
text, YAML, and binary content. Fourteen operations have a success status other
than 200. The global bearer security requirement has six operation overrides.

This is wider than RIG's current typed-function model. In particular, Holder
needs metadata for operation summaries and descriptions, security exceptions,
response descriptions and content types, literal field values, optional record
fields, nullable values, non-200 success statuses, and header parameters. Some
routes also have dynamic response bodies and streaming behavior. We should add
these features to RIG as general capabilities, not as Holder-specific output
patches.

## First live slice: `POST /bye`

`daemon/src/api/Bye.h` declares the typed operation and response. The existing
HTTP route in `HttpServer.cpp` calls that operation, so the reflected declaration
describes live behavior. `RigAnnotations.h` makes its annotation disappear in
ordinary C++20 builds.

With an installed RIG command and PyYAML, run the comparison from the repo
root:

```sh
make -C api/rig check
```

To inspect the intermediate artifacts directly:

```sh
rig inspect --header daemon/src/api/Bye.h --namespace holder::api -- -Idaemon/src > /tmp/holder-bye-rig.json
rig openapi --title 'Holder API' --omit-operation-ids < /tmp/holder-bye-rig.json > /tmp/holder-bye-openapi.json
```

The generated `/bye` operation and its three reachable schemas now match the
control structurally, including the absence of an operation ID and the open
record schemas. The generated document remains a partial slice; it must not
replace the checked-in contract yet.

The next slice should describe service-level bearer security and the document
metadata, then use the same mechanism on a route with path and query
parameters.
