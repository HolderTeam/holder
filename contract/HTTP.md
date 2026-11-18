# The generated HTTP routes

HTTP is one expression of [Holder's contract](README.md). Python declarations define the public operations and their HTTP bindings; FastAPI produces the basis of the OpenAPI specification. The daemon implements those operations and serves the generated specification to clients.

Generation supplies dispatch, validation and JSON codecs around implementations. The daemon supplies the services and connections those implementations need. The design is for request and response shapes to come from declarations wherever an operation is implemented.

Run `make -C contract update` after changing declarations or generators. The HTTP outputs live in [`api/`](../api/) and [`daemon/src/api/generated/`](../daemon/src/api/generated/). API metadata and shared security settings come from [`api/openapi-base.json`](../api/openapi-base.json). See [generator responsibilities](generators/README.md) for work on the tooling, and [the HTTP client guide](../api/README.md) for consuming and versioning the interface.

## Routes

```python
contract = Contract(errors={NotFound: (404, 4040)})

@contract.route("GET", "/projects/{project_id}/cards/{card_id}", errors=[NotFound])
def get_card(project_id: Annotated[ProjectId, Path()], card_id: Annotated[CardId, Path()]) -> Card: ...

@contract.route("POST", "/projects/{project_id}/cards", status=201, errors=[NotFound])
def create_card(project_id: Annotated[ProjectId, Path()], card: Annotated[NewCard, Body()]) -> Card: ...
```

- Each parameter says where it comes from: `Path()`, `Query()`, `Header()` or `Body()`.
- `status` is the success status, 200 unless given; it must be a 2xx with a body. `created_when` names a boolean result field that selects 201 when true and 200 when false.
- A route's `errors` lists its error kinds; the `Contract` error table gives each kind its HTTP status and C error code, side by side. Every error kind needs both. The error kinds name conditions, such as `NotFound`; only `app.py` says what they are in HTTP and in C.

## Conventions

These are the conventions for generated core-backed JSON routes. Declarations can also specify raw responses, binary content and event streams; their response forms appear in OpenAPI.

- **Success for an ordinary JSON operation:** `{"ok": true, "data": <result>}`, with the route's status. In OpenAPI, the envelope of a result is a schema of its own, such as `CardResponse`.
- **Failure:** `{"ok": false, "error": {"code": <code>, "message": <text>}}`:
  - an argument the contract rejects is 400, with the code `invalid_argument`;
  - an error kind has the status the `Contract` gives it, and its name in snake case as the code, such as `not_found`;
  - anything else is 500, `internal_error`.
- **Cards** are named by their project and their ID, under `/projects/{project_id}/cards/{card_id}`, because copies of a project, such as a fork, keep their card IDs. A card in another project is not found. In the C API, `project_id` comes before `card_id`.
- **Routes:** a literal path segment is tried before a parameter, and the generator rejects routes that could match the same request ambiguously.
- **Path and query values** are percent-decoded; in the query, `+` is a space. A query parameter given twice, or one the operation does not have, is an invalid argument. Booleans are `true`, `false`, `1` or `0`. A choice of literals is its value as text, and `parent=roots` is the literal beside a card ID.
- **Bodies:** a body whose record's fields may all be left out may itself be left out, and means `{}`. An operation without a body ignores one.
- **Text** is UTF-8 without U+0000, and its length constraints count characters, as in the contract and the C API.
- **Records** have only the fields they declare, in requests and results alike, and the OpenAPI says so (`additionalProperties: false`). A `card_id` sent to create a card, which Holder names, is an invalid argument rather than silently ignored. Clients should still read results leniently, ignoring fields they do not know, because a client can be older or newer than the daemon it talks to. A field that may be omitted must not be null unless it is nullable. A record that extends another lists the other's fields as its own, as Pydantic writes it, rather than as an `allOf`, which closed schemas could not satisfy.
- **Other methods:** `dispatch` answers only requests it has an operation for. For a path it knows, with another method, it returns nothing, so that another router with routes on the same path can answer; `allowed_methods(path)` lists the methods it serves there, for a 405's `Allow` header.

## Complete daemon-only boundary proof

`PUT /locations/{location_id}/binding` is the first complete daemon-only boundary. [`boundary.py`](generators/daemon/boundary.py) validates its explicit runtime binding against the compiler result. It generates the reachable C++ record definitions and handler declaration in `DaemonTypes.h`, codecs in `DaemonJson.h`, and dispatch in `DaemonRoutes.h`. Error-response fields come from the compiler-produced FastAPI artifact. No generator loads Python declarations.

The runtime binding supplies the C++ handler name, header, service dependencies and exception-to-declared-status mappings. `LocationWrite.cpp` retains database access, secret storage, provider behavior and the `Configured` fallback when an optional preview is omitted. The generated boundary retains its existing 200/400/404/503/500 statuses and error codes, and shares the response codec with the remaining location actions. Authentication and 405/Allow handling remain in the existing daemon runtime.

The supported subset is explicit: closed, non-generic records containing strings, booleans, string maps, named records and nullable/omittable fields, with string length constraints and scalar string/boolean/null defaults. Omitted fields and explicit null remain distinct, including nested optional results. Unsupported types, constraints, defaults, response schemas/statuses or missing runtime bindings fail generation. Other operations are not silently treated as complete.

Generated validation rejects unknown fields, invalid JSON/types, duplicate/unknown queries, and text outside the contract's UTF-8/no-U+0000 rules. This includes map keys and values, which the old binding decoder only checked for JSON string type. Valid binding requests and preview behavior are unchanged; contract-invalid map text is rejected before secret-store access.

[`DaemonBoundaries.md`](../daemon/src/api/generated/DaemonBoundaries.md) inventories every daemon-only operation, its actual handler, remaining boundary work and the next Desktop-related migrations. Freshness checks regenerate this inventory and reject missing or stale dispositions. Existing AI dispatch/codecs are listed as partially generated because their runtime types and remaining validation conventions still need review. Core-backed card and project sync operations already use the generated core boundary.

`tests/test_daemon_boundary.py` compiles and runs generated code with an independent runtime stand-in, including newly added fields and default/null/omission behavior. `tests/conformance/test_location_binding.py`, enabled with `HOLDER_DAEMON`, exercises the actual daemon's accepted/rejected requests, preview state, secret non-disclosure, authentication and method handling. Native route tests cover unavailable and failing secret stores. Skipped live tests are not counted as executed runtime evidence.
