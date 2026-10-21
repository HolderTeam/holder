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

`daemon/src/api/ApiService.h` declares document metadata and bearer security,
including the 401 and 500 responses every operation documents. `Session.cpp`
skips the token check for operations the generated dispatcher reports as
`RIG_PUBLIC`.
`daemon/src/api/Bye.h` declares the typed operation and response.
`daemon/src/api/Alive.h` declares `GET /alive`, the first `RIG_PUBLIC`
operation: a JSON liveness check that needs no token.
`daemon/src/api/RigRuntime.h` selects the migrated operations for one
checked-in C++20 JSON codec and dispatcher in `daemon/src/api/generated/`.
`Session.cpp` calls the dispatcher after authentication and event handling. A
no-match result falls through to the registered and legacy routes.
`RigAnnotations.h` makes annotations disappear in ordinary C++20 builds.

Each `RIG_ERROR_RESPONSE` names the exception types that produce it, and the
generated dispatcher catches them and returns that status. `rig_error_body` in
`GeneratedRoutes.cpp` builds the `ErrorResponse`; `PrivacyError` keeps its own
code. `ApiService.h` declares `ErrorResponse` as the validation error body, so
RIG's own request-decoding 400s use the same envelope and `rig_error_body`, and
every operation with HTTP input documents them. Each operation's unmapped
`std::exception` becomes a generated 500; `GeneratedRoutes.cpp` only guards
against a failure outside the operations reaching the worker.

`daemon/src/api/ProjectTags.h` declares `GET /projects/{project_id}/tags`.
The shared dispatcher handles this route and its JSON response. Holder supplies
the database context in `GeneratedRoutes.cpp`. The old handwritten route and JSON response were removed.
RIG omits the server-supplied context from OpenAPI while reflecting the path
parameter, error responses, field descriptions, and integer minimum.

`daemon/src/api/Rebuild.h` declares `POST /rebuild` with a required JSON request
record. The generated dispatcher decodes the body and encodes the response;
the typed operation calls core's rebuilder. Missing projects map to 404
and rebuild failures to 400. The old route wrapper was removed.

`daemon/src/api/ProviderSettings.h` declares GET and PUT on
`/ai/providers/settings` and DELETE on `/ai/providers/settings/{provider}`.
The shared dispatcher lists settings, decodes PUT's optional timestamp, and
decodes DELETE's provider path parameter. The typed mutations normalize the
provider and persist the change. The list and delete responses use inline
records for `data`, matching the control's nested object schemas.

`daemon/src/api/ProviderCredentials.h` declares GET and PUT on
`/ai/providers/credentials`. The typed operations list masked credential
previews and store new keys through Holder's secret store context. RIG decodes
the optional nullable timestamp and encodes the response. DELETE still uses
the existing secret-store handler.

`daemon/src/api/RunnerPull.h` declares `GET /ai/runner/pull/{job_id}` with an
optional `runner_id` query parameter. The shared dispatcher decodes the query
and path, calls the typed operation with Holder's runner registry, and encodes
the response. Runner lookup errors map to the existing 404 response.
The response covers 64-bit integers, a double, omittable fields, and an
omittable nullable field.

`daemon/src/api/CardLinks.h` declares `GET /cards/{card_id}/links` with an
optional integer query parameter constrained to `0` or `1`. The live handler
is now the shared dispatcher: it decodes and validates the query, calls the
typed operation, and encodes the response. The operation resolves the source
card's project from the database before listing links. POST and DELETE still
use the existing card route. The reflected link response covers optional
properties and an optional nullable label.

With an installed RIG command, PyYAML and core's installed SDK (the daemon's
build installs it under `core/out/install/local`; set `CORE_INCLUDE` to use
another), run the comparison from the repo root:

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

The generated service metadata, bearer security, twelve operations, and their
twenty-six reachable schemas now match the control. The comparison treats an omitted
`required` on a query parameter as `false`, as OpenAPI does. The generated
document remains a partial slice; it must not replace the checked-in contract
yet.

The card reference resolver exercises string choices on record fields and
outcome dependent response fields through the generated route and JSON codec.
