# The generated HTTP routes

`interface/http.py` binds the contract's operations to HTTP, and the daemon's generators turn the bindings into three files:

- **`ContractRoutes.h`:** `contract_http::dispatch`, which matches a request to an operation, decodes and checks its arguments, calls core's C++ function for it with the caller's `holder_context`, and encodes the result.
- **`ContractJson.h`:** the JSON codec for the contract's C++ types (`core/src/api/Contract.h`).
- **`openapi.json`:** the operations' OpenAPI paths and the schemas they use, to be merged into the daemon's document, which adds authentication, servers and the API's metadata.

`python -m holder_contract generate --output <directory>` writes them, with core's C API files. The generators were ported from [RIG](https://github.com/zeth/rig) at `e32ea68` (`http_routes.py`, `json_codec.py`, `openapi.py` and the route rules in `ir.py`), which generated the same kinds of file from declarations reflected from C++. They now read the contract's IR instead. `tests/http/` compiles the routes against the stand-in for core in `tests/c/` and sends them requests.

## Binding

```python
http_error(NotFound, 404)

http(get_card, "GET", "/cards/{card_id}")
http(list_cards, "GET", "/projects/{project_id}/cards", query=["parent", "tag", "order", "limit", "cursor"])
http(create_card, "POST", "/cards", status=201)
```

- A parameter named in the path comes from the path, one listed in `query` from the query string, and at most one other, a record, is the JSON request body.
- `status` is the success status, 200 unless given; it must be a 2xx with a body.
- `http_error(kind, status)` gives an error kind its status. Every error kind of an HTTP-bound operation needs one. The error kinds name conditions, such as `NotFound`; only this file says what they are in HTTP.

## Conventions

- **Success:** `{"ok": true, "data": <result>}`, with the binding's status.
- **Failure:** `{"ok": false, "error": {"code": <code>, "message": <text>}}`:
  - an argument the contract rejects is 400, with the code `invalid_argument`;
  - an error kind has the status `http_error` gives it, and its name in snake case as the code, such as `not_found`;
  - anything else is 500, `internal_error`.
- **Routes:** a literal path segment is tried before a parameter, and the generator rejects routes that could match the same request ambiguously.
- **Path and query values** are percent-decoded; in the query, `+` is a space. A query parameter given twice is an invalid argument. Booleans are `true`, `false`, `1` or `0`. A choice of literals is its value as text, and `parent=roots` is the literal beside a card ID.
- **Text** is UTF-8 without U+0000, and its length constraints count characters, as in the contract and the C API.
- **Records** in a request may carry fields the contract does not know; they are ignored, as the contract's records allow. A field that may be omitted must not be null unless it is nullable.
- **Other methods:** `dispatch` answers only requests it has an operation for. For a path it knows, with another method, it returns nothing, so that another router with routes on the same path can answer; `allowed_methods(path)` lists the methods it serves there, for a 405's `Allow` header.
