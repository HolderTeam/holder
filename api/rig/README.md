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
`daemon/src/api/Bye.h` declares the typed operation and response. RIG generates
the checked-in C++20 JSON codec and route dispatcher in
`daemon/src/api/generated/`; `Router.cpp` calls that dispatcher after its
existing routes. The daemon's bearer-auth gate still runs before dispatch.
`RigAnnotations.h` makes annotations disappear in ordinary C++20 builds.

`daemon/src/api/ProjectTags.h` declares `GET /projects/{project_id}/tags`.
RIG generates its checked-in C++20 dispatcher and JSON codec. Holder supplies
the database context in `ProjectTagDispatch.cpp`, which also maps domain errors
to HTTP responses. The old handwritten route and JSON response were removed.
RIG omits the server-supplied context from OpenAPI while reflecting the path
parameter, error responses, field descriptions, and integer minimum.

`daemon/src/api/RunnerPull.h` declares `GET /ai/runner/pull/{job_id}` with an
optional `runner_id` query parameter. The live handler calls the typed
operation. Its response covers 64-bit integers, a double, omittable fields,
and an omittable nullable field.

`daemon/src/api/CardLinks.h` declares `GET /cards/{card_id}/links` with an
optional integer query parameter constrained to `0` or `1`. The live handler
calls the typed operation and rejects other supplied values. The reflected
link response covers optional properties and an optional nullable label.

With an installed RIG command and PyYAML, run the comparison from the repo
root:

```sh
make -C api/rig check
make -C api/rig check-generated
```

After changing `/bye` annotations, run `make -C api/rig update-generated` and
commit the generated headers. These commands require RIG only while developing
the contract; normal daemon builds compile the checked-in headers.

To inspect the intermediate artifacts directly:

```sh
rig inspect --header daemon/src/api/RigContract.h --namespace holder::api --namespace holder::api::routes -- -Idaemon/src > /tmp/holder-rig.json
rig openapi --omit-operation-ids < /tmp/holder-rig.json > /tmp/holder-openapi.json
```

The generated service metadata, bearer security, four operations, and their
ten reachable schemas now match the control. The comparison treats an omitted
`required` on a query parameter as `false`, as OpenAPI does. The generated
document remains a partial slice; it must not replace the checked-in contract
yet.

The next full-stack slice can cover an operation with a request body or query
parameter, exercising generated input decoding alongside output encoding.
