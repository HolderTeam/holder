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

With an installed RIG command, inspect and generate this slice from the repo
root:

```sh
rig inspect --header daemon/src/api/Bye.h --namespace holder::api -- -Idaemon/src > /tmp/holder-bye-rig.json
rig openapi --title 'Holder API' < /tmp/holder-bye-rig.json > /tmp/holder-bye-openapi.json
```

The generated path, method, summary, long description, 200 response (including
its description and JSON schema reference), response type, and required
`ok: true` field now agree with the control. The generated fragment still
misses the 401 response. It adds an `operationId` and
`additionalProperties: false` to the response schema, neither of which the
control declares. These are concrete RIG gaps; the generated fragment must
not replace the checked-in contract yet.

The next slice should settle how RIG represents service-level authentication
and error responses, then make operation IDs and record openness explicit so
this operation can match the control exactly. After that, use the same
mechanism on a route with path and query parameters.
