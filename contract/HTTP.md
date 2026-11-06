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
