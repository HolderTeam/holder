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
`daemon/src/api/Bye.h` declares the typed operation and response.
`daemon/src/api/RigRuntime.h` selects the migrated operations for one
checked-in C++20 JSON codec and dispatcher in `daemon/src/api/generated/`.
`Session.cpp` calls the dispatcher after authentication and event handling. A
no-match result falls through to the registered and legacy routes.
`RigAnnotations.h` makes annotations disappear in ordinary C++20 builds.

`daemon/src/api/ProjectTags.h` declares `GET /projects/{project_id}/tags`.
The shared dispatcher handles this route and its JSON response. Holder supplies
the database context in `GeneratedRoutes.cpp`, which also maps domain errors
to HTTP responses. The old handwritten route and JSON response were removed.
RIG omits the server-supplied context from OpenAPI while reflecting the path
parameter, error responses, field descriptions, and integer minimum.

`daemon/src/api/Rebuild.h` declares `POST /rebuild` with a required JSON request
record. The generated dispatcher decodes the body and encodes the response;
the typed operation calls core's rebuilder. Holder maps missing projects to 404
and rebuild failures to 400. The old route wrapper was removed.

`daemon/src/api/ProviderSettings.h` declares `PUT /ai/providers/settings` with
an optional timestamp in its JSON request. The shared dispatcher decodes and
encodes this operation; its typed function normalizes the provider and persists
the setting. GET and DELETE on the settings path remain in the existing handler.

`daemon/src/api/RunnerPull.h` declares `GET /ai/runner/pull/{job_id}` with an
optional `runner_id` query parameter. The shared dispatcher decodes the query
and path, calls the typed operation with Holder's runner registry, and encodes
the response. Holder maps runner lookup errors to its existing 404 response.
The response covers 64-bit integers, a double, omittable fields, and an
omittable nullable field.

`daemon/src/api/CardLinks.h` declares `GET /cards/{card_id}/links` with an
optional integer query parameter constrained to `0` or `1`. The live handler
is now the shared dispatcher: it decodes and validates the query, calls the
typed operation, and encodes the response. The operation resolves the source
card's project from the database before listing links. POST and DELETE still
use the existing card route. The reflected link response covers optional
properties and an optional nullable label.

With an installed RIG command and PyYAML, run the comparison from the repo
root:

```sh
make -C api/rig check
make -C api/rig check-generated
```

After changing a migrated operation, run `make -C api/rig update-generated` and
commit the generated headers. These commands require RIG only while developing
the contract; normal daemon builds compile the checked-in headers.

To inspect the intermediate artifacts directly:

```sh
rig inspect --header daemon/src/api/RigContract.h --namespace holder::api --namespace holder::api::routes -- -Idaemon/src > /tmp/holder-rig.json
rig openapi --omit-operation-ids < /tmp/holder-rig.json > /tmp/holder-openapi.json
```

The generated service metadata, bearer security, seven operations, and their
twenty reachable schemas now match the control. The comparison treats an omitted
`required` on a query parameter as `false`, as OpenAPI does. The generated
document remains a partial slice; it must not replace the checked-in contract
yet.

The card reference resolver exercises string choices on record fields and
outcome dependent response fields through the generated route and JSON codec.
