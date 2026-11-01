# The generated HTTP routes

`interface/app.py` declares the contract's operations as FastAPI routes, and the daemon's generators turn them into the route and codec files and the API document:

- **`ContractRoutes.h`:** `contract_http::dispatch`, which matches a request to an operation, decodes and checks its arguments, calls core's C++ function for it with the caller's `holder_context`, and encodes the result.
- **`ContractJson.h`:** the JSON codec for the contract's C++ types (`core/src/api/Contract.h`).
- **`AiRoutes.h`:** dispatch for daemon owned AI handlers, with paths, arguments and status codes from the Python contract.
- **`AiJson.h`:** the JSON codec for the daemon's AI records (`interface/ai/models.py`).
- **`openapi.json`:** the operations' OpenAPI paths and the schemas they use: FastAPI's document of `app.py`, with FastAPI's own 422 validation responses and the operation IDs removed, the daemon's error schemas, and each schema named by its title, so that a generic record's use is `CardListItemPage` rather than Pydantic's `Page_CardListItem_` (`generators/daemon/openapi.py`).

`make -C contract update` writes the four C++ headers into `daemon/src/api/generated/` and merges the OpenAPI paths with `api/openapi-base.json`, which supplies API metadata and security, to write `api/openapi.yaml` and `api/openapi.json`. `make -C contract check` fails when any generated file is out of date. `python -m holder_contract generate --output <directory>` writes every generated file, including core's C API files, into one directory.

The daemon's AI handlers use `AiRoutes.h` and `AiJson.h`, generated from `interface/app.py` and `interface/ai.py`. A contract test verifies their route inventory. Other routes are dispatched directly or through `contract_http::dispatch` (`daemon/src/api/GeneratedRoutes.cpp`). Core-backed requests run with a `holder_context` on the worker's database, full-text indexer and Git service. The contract's routes and codec generators were ported from [RIG](https://github.com/zeth/rig) at `e32ea68`; they now read the Python contract's IR. `tests/http/` compiles the routes against the stand-in for core in `tests/c/` and sends them requests.

## Routes

```python
contract = Contract(errors={NotFound: (404, 4040)})

@contract.route("GET", "/projects/{project_id}/cards/{card_id}", errors=[NotFound])
def get_card(project_id: Annotated[ProjectId, Path()], card_id: Annotated[CardId, Path()]) -> Card: ...

@contract.route("POST", "/projects/{project_id}/cards", status=201, errors=[NotFound])
def create_card(project_id: Annotated[ProjectId, Path()], card: Annotated[NewCard, Body()]) -> Card: ...
```

- Each parameter says where it comes from: `Path()`, `Query()`, or `Body()` for at most one, a record, as the JSON request body.
- `status` is the success status, 200 unless given; it must be a 2xx with a body. `created_when` names a boolean result field that selects 201 when true and 200 when false.
- `errors` gives each error kind its HTTP status and its C error code, side by side. Every error kind needs both. The error kinds name conditions, such as `NotFound`; only `app.py` says what they are in HTTP and in C.

## Conventions

- **Success:** `{"ok": true, "data": <result>}`, with the route's status. In OpenAPI, the envelope of a result is a schema of its own, such as `CardResponse`.
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
