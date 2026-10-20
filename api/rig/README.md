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

## Live slices

`daemon/src/api/ApiService.h` declares document metadata and bearer security.
`daemon/src/api/Bye.h` declares the typed operation and response. The existing
HTTP route in `HttpServer.cpp` calls that operation, so the reflected declaration
describes live behavior. `RigAnnotations.h` makes these annotations disappear
in ordinary C++20 builds.

`daemon/src/api/ProjectTags.h` declares `GET /projects/{project_id}/tags`.
The live handler calls its typed function with a database context. RIG omits
that server-supplied parameter from the HTTP contract. Its path parameter,
error responses, field descriptions, and integer minimum are reflected.

`daemon/src/api/RunnerPull.h` declares `GET /ai/runner/pull/{job_id}` with an
optional `runner_id` query parameter. The live handler calls the typed
operation. Its response covers 64-bit integers, a double, omittable fields,
and an omittable nullable field.

With an installed RIG command and PyYAML, run the comparison from the repo
root:

```sh
make -C api/rig check
```

To inspect the intermediate artifacts directly:

```sh
rig inspect --header daemon/src/api/RigContract.h --namespace holder::api -- -Idaemon/src > /tmp/holder-rig.json
rig openapi --omit-operation-ids < /tmp/holder-rig.json > /tmp/holder-openapi.json
```

The generated service metadata, bearer security, three operations, and their
eight reachable schemas now match the control. The comparison treats an omitted
`required` on a query parameter as `false`, as OpenAPI does. The generated
document remains a partial slice; it must not replace the checked-in contract
yet.

The next slice should cover an enum or constrained query parameter.
