# The contract's IR, version 1

`python -m holder_contract ir` reads the contract and writes one JSON object, the input of every generator. The same contract always gives the same bytes. `tests/fixtures/sample.ir.json` is a complete example.

```json
{
  "ir_version": 1,
  "aliases": [],
  "enums": [],
  "records": [],
  "errors": [],
  "interfaces": [],
  "operations": [],
  "http": []
}
```

Declarations appear in the order of their modules' paths, then in source order. Every declared name is unique across the contract. Each declaration has a `name`, a `source` (its file, relative to `contract/`, for the "generated from" line of generated files) and a `doc`.

## Declarations

- **Alias** (a `type` statement): `name`, `source`, `doc`, `type`.
- **Enum** (a `StrEnum`): `name`, `source`, `doc`, and `values`, each with the member's `name` and its string `value`.
- **Record**:
  - `name`, `source`, `doc`, and its own `fields`;
  - `base`, the record it extends, if any. Its fields are listed with that record.
  - `type_parameters`, for a generic record such as `Page[T]`; its fields refer to them with `{"kind": "parameter"}`.
- **Field**: `name`, `doc` and `type`.
  - `omittable: true` when the field may be left out (`MISSING`).
  - `default` when it has one.
  - A field with neither is required.
- **Error kind**: `name`, `source`, `doc`, and `fields`, which have no defaults.
- **Interface**: `name`, `source`, `doc`, and `methods`, each a signature.
- **Operation**: a signature, plus `source`, `errors` (the names of its error kinds) and `daemon_only`.
- **Signature**: `name`, `summary`, `description`, `parameters` and `returns`. Each parameter has a `name` and `type`, and may have a `doc` and a `default`.
- **HTTP binding**: `operation`, `method`, `path`, `path_parameters` and `query`, plus `body` when one parameter is left for the request body.

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

A type may also carry constraints: `min_length`, `max_length`, `pattern`, `ge`, `le`, `gt` and `lt`.

Omission is not a type. It belongs to a record's field, as `omittable`.
