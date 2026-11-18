# The contract's IR, version 1

The compiler's intermediate representation (IR) describes types and operations independently of an output language. Generators use it to derive target interfaces. See [generator responsibilities](generators/README.md) for that boundary and [the contract overview](README.md) for the architecture.

`python -m compiler ir` writes the representation as JSON for inspection. [`compiler/ir.py`](compiler/ir.py) defines its schema; `tests/fixtures/sample.ir.json` provides an example.

```json
{
  "ir_version": 1,
  "aliases": [],
  "enums": [],
  "records": [],
  "errors": [],
  "interfaces": [],
  "operations": [],
  "http": [],
  "http_errors": [],
  "constants": []
}
```

`constants` is left out when the contract declares none.

When the package has an `app.py`, the result also includes `openapi`: a detached JSON document produced by FastAPI during compilation. It retains FastAPI's paths, schemas, response descriptions, headers, content and additional statuses, including bodyless responses. It contains no Python models or callables. A package without an app leaves this artifact out; native generation does not need it. The daemon's OpenAPI backend requires it and applies Holder's HTTP conventions before merging API metadata and security. FastAPI remains responsible for schema production; native and Android interfaces use the typed declarations.

Persist a result with `model_dump_json(exclude_unset=True)` (as the `ir` command does), and restore it with `ContractView.model_validate_json`. Leaving unset fields out preserves the distinction between an absent default and an explicit default of `None`.

Model declarations follow `DECLARATION_ORDER` in the declaration package, then source order within each module. Every declared name is unique across the contract. Each declaration has a `name`, a `source` (its file, relative to `contract/`, for the "generated from" line of generated files) and a `doc`.

## Declarations

- **Alias** (a `type` statement): `name`, `source`, `doc`, `type`.
- **Constant** (`NAME: Final = "..."` in a model module): `name`, `source`, `doc` and its string `value`. A constant is text the contract fixes, such as a default name, so every platform takes it from one place.
- **Enum** (a `StrEnum`): `name`, `source`, `doc`, and `values`, each with the member's `name` and its string `value`.
- **Record**:
  - `name`, `source`, `doc`, and its own `fields`;
  - `base`, the record it extends, if any. Its fields are listed with that record.
  - `open: true` for an `OpenRecord`, which permits fields outside the declared schema.
  - `type_parameters`, for a generic record such as `Page[T]`; its fields refer to them with `{"kind": "parameter"}`.
- **Field**: `name`, `doc` and `type`.
  - `omittable: true` when the field may be left out (`MISSING`).
  - `default` when it has one.
  - A field with neither is required.
- **Error kind**: `name`, `source`, `doc`, `fields`, which have no defaults, and `code`, its C error code from the contract's error table (left out when the contract has no `app.py`).
- **Interface**: `name`, `source`, `doc`, and `methods`, each a signature.
- **Operation**: a signature, plus `source`, `errors` (the names of its error kinds) and `daemon_only`. A runtime operation also has `context`: `none` when it runs without a context, `opens` when it makes one, and `closes` when it releases the one it is given; it is left out for the ordinary operation, which works through a context. An operation on an error core returned, rather than on a context, also has `error`: `reads` when it reads the error's message, `closes` when it releases it. Operations are read from domain modules registered by the contract's `app.py`, in registration order. `source` names the module that declares the operation.
- **Signature**: `name`, `summary`, `description`, `parameters` and `returns`. Each parameter has a `name` and `type`, and may have a `doc` and a `default`.
- **HTTP binding**: `operation`, `method`, `path`, `path_parameters`, `query` and `status` (the success status), plus `body` when one parameter is left for the request body, `headers` when header parameters are declared, and `public: true` for an unauthenticated route. `created_when` names a boolean result field that selects 201 when true, leaving `status` (200) for false. Each comes from FastAPI's analysis of a route declared in a domain module.
- **HTTP error status** (in `http_errors`): an error kind's `error` name and its `status`.

HTTP bindings also retain `operation_id` when explicitly supplied, `response_media_type`, and `response_mode`: `envelope` (the default), `raw` or `stream`. Raw responses may have `raw_schema`; an empty object is distinct from no override. `sse_events` maps each wire event name to a named record reference. Event-only payload records are therefore reachable without interpreting the Python declaration again. Named events require a stream result and declared, non-generic record payloads. Raw schemas require a raw response (an explicit raw media type or a binary result) and JSON-compatible values. Target backends reject any further shapes their conventions cannot express.

The compiler/backend metadata inventory is:

| Authoring input | Compilation result | Backend use |
|---|---|---|
| Models, aliases, enums, constants, constraints and defaults | Typed declarations and type references | Native declarations, codecs and Android/JNI |
| Signatures, errors, core availability, context/error lifetimes and host interfaces | Operations, errors and interfaces | Native ABI and runtime adapters |
| Method, path, status, parameter locations and body | HTTP bindings | HTTP dispatch and clients |
| `created_when`, public access and explicit operation ID | HTTP binding metadata; FastAPI artifact includes additional success status | Status selection and OpenAPI conventions |
| Raw media type, binary return and raw schema override | Response mode, media type, raw schema and FastAPI artifact | Raw/binary OpenAPI responses |
| Named SSE events | HTTP event-name-to-record map and FastAPI payload schemas | Event codecs/helpers and OpenAPI SSE item schemas |
| Additional response models, statuses, descriptions, headers and content | Resolved paths and component schemas in `openapi` | OpenAPI response declarations |

The compiler owns the interpretation of every input in this table. Naming, transport conventions, runtime service wiring, formatting, file layout and API metadata/security merging remain backend responsibilities.

## Types

Every `type` is an object with a `kind`:

| `kind` | Members | From |
|---|---|---|
| `string`, `bool` | | `str`, `bool` |
| `int32`, `int64`, `float64` | `format` (`unix-seconds` for a `Timestamp`) | `Int32`, `Int64`, `Timestamp`, `Float64` |
| `json` | | `JsonValue` |
| `secret` | | `SecretStr` |
| `none` | | `None`, only as a result |
| `literal` | `values`: all strings or all integers | `Literal[...]` |
| `list` | `items` | `list[T]` |
| `map` | `values` (keys are strings) | `dict[str, T]` |
| `nullable` | `value` | `T \| None` |
| `union` | `alternatives`, and `discriminator` when one field tells them apart | `A \| B` |
| `named` | `name`, and `arguments` for a generic record | a record, enum or alias |
| `parameter` | `name` | a generic record's type parameter |
| `stream` | `items` | `Stream[T]`, only as a result |
| `bytes` | | `Bytes`, only as a result |

A type may also carry constraints: `min_length`, `max_length`, `pattern`, `ge`, `le`, `gt` and `lt`. Lengths count characters.

Every `string` is Unicode text without U+0000, and every `float64` is finite; neither rule is written in the IR.

Omission is not a type. It belongs to a record's field, as `omittable`.
