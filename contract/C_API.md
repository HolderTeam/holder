# The generated C API

The C API is the native expression of [Holder's contract](README.md). It lets applications embed core using the same declared operations and types that underlie shared HTTP operations. The generator supplies C calling conventions, validation, error translation and memory ownership; core implements the domain behavior.

The public header is [`holder_api.h`](../core/include/holder/holder_api.h). Generated C++ declarations and glue connect it to implementations in [`core/src/api/`](../core/src/api/). Change the Python declarations or generators, then run `make -C contract update`; keep the regenerated files with the change. See [core's consumption model](../core/README.md#consumption-model) for building against the library and its compatibility policy.

## Implementation boundary

The core generator writes these files:

- [`holder_api.h`](../core/include/holder/holder_api.h): the public C declarations for handles, records, operations, host callbacks, error codes and result arenas.
- [`Contract.h`](../core/src/api/Contract.h): the C++ types and operation declarations core implements. They live in `holder::contract` so they do not clash with the daemon's `holder::api` types. A missing operation implementation fails to link when the generated glue is linked into a consumer.
- [`ContractCApi.cpp`](../core/src/api/ContractCApi.cpp): C entry points that validate arguments, call core's implementations, copy results into the caller's arena and translate exceptions into C errors.
- [`HostCApi.cpp`](../core/src/api/HostCApi.cpp): adapters for host-implemented interfaces, connecting C callbacks and their ownership conventions to core's C++ interfaces.

The glue rejects null pointers where a value is required, numbers outside declared bounds and unknown enum values before calling core. Core can rely on those argument checks; it remains responsible for domain rules that depend on the system's state.

A generated C++ record inherits the record it extends: `holder::contract::Card` derives from `CardMetadata`. Core reuses the generated metadata type through the alias `holder::model::Card = holder::contract::CardMetadata`, keeping one definition of those fields.

The generated C++ interface uses `long long` for 64-bit integers; the public C interface uses `int64_t`. The glue converts between them at the boundary.

The generator tests compile and run generated code against a stand-in for core in [`tests/c/`](tests/c/). This checks the calling conventions, validation, conversions and memory handling independently of core's domain behavior. Core's own tests and the [conformance tests](tests/conformance/) exercise the real implementation.

## Conventions

- **Operations:**
  - Every operation is `int holder_<operation>(holder_context* context, <parameters>, holder_arena* arena, const <result>** out_<result>, holder_error** out_error)`.
  - It returns `HOLDER_OK` (0) or an error code, and sets `*out_error` as the host registration functions do (free it with `holder_error_destroy`).
  - An operation with no result has no `out_` parameter and no `arena`, which only holds results.
  - An operation that returns a list returns it as a `{items, count}` struct, in `out_<record>_list`, such as `const holder_tag_count_list** out_tag_count_list`.
  - An operation on a card takes the card's `project_id` before its `card_id`, and does not find a card in another project.
  - **Runtime operations** (`interface/runtime/`) are declared with how they use a context:
    - one that runs without a context, such as `holder_version` and `holder_database_rebuild`, drops the `context` parameter;
    - one that opens a context is `int holder_<operation>(<parameters>, holder_context** out_context, holder_error** out_error)`, as `holder_context_open` is: it takes no arena, and the caller owns the new context;
    - one that closes a context is `void holder_<operation>(holder_context* context)`, as `holder_context_destroy` is;
    - one that reads an error core returned is `const char* holder_<operation>(const holder_error* error)`, as `holder_error_message` is, and one that releases it is `void holder_<operation>(holder_error* error)`, as `holder_error_destroy` is.
- **Error codes:** a code is the HTTP status it echoes and one more digit, so it reads as that status. `holder_api.h` declares `HOLDER_ERROR_INVALID_ARGUMENT` (4000) for anything the contract's checks reject, `HOLDER_ERROR_RUNTIME` (5000) for an unexpected failure, and `HOLDER_ERROR_ALLOCATION` (5001). Each error kind in the contract has the code its error table gives it, such as `HOLDER_ERROR_NOT_FOUND` (4040); a code keeps its meaning once released, wherever its declaration moves.
- **Memory:** results live in the `holder_arena` the caller passes, until `holder_arena_free`. Nothing in a result needs freeing on its own.
- **Strings** are NUL-terminated UTF-8. The glue rejects text that is not well-formed UTF-8, and length constraints count characters, not bytes.
- **Numbers:** the glue rejects a `double` that is NaN or infinite.
- **Nullable and optional values:**
  - A nullable or absent string is `NULL`.
  - A nullable, absent or defaulted number in a record is a pointer, `NULL` when absent.
- **Partial updates:** a field of an input record that is both nullable and omittable, such as a card's new parent in an update, is a `holder_string_change`: `{change, value}`, where `change` is `HOLDER_KEEP` (zero, so a zeroed record changes nothing), `HOLDER_SET` (to `value`) or `HOLDER_CLEAR` (to null). In C++ it is a `Change<T>`, `std::optional<std::optional<T>>`: `nullopt` keeps the field, an empty value clears it.
- **Lists:** a list is a `{items, count}` struct.
- **Parameter defaults:**
  - A parameter whose default is the zero value (`false`, `0`) is passed by value, so zero means the default.
  - Any other defaulted number is a pointer, `NULL` for the default.
  - A defaulted string is `NULL` for the default.
- **Choices:**
  - A choice of literals is an enum. Its zero member is `…_DEFAULT` when the parameter has a default, and the other members start at 1.
  - A choice between literals and a string, such as `"roots"` or a card ID, is a `{kind, value}` struct whose zero kind means null.
  - A required choice of strings in an input record is an enum named for the record and the field, such as `holder_card_move_intent`, whose members start at 1; the glue rejects zero. In C++ it is an `enum class` (`CardMoveIntent`).
  - Every enum ends with `…_MAX_ENUM = 0x7FFFFFFF`, so any `int` is a valid value of the type and the glue can reject a bad one without undefined behaviour.
- **Input records** are passed by pointer. A zeroed record means every optional field is absent: `holder_new_card card = {0};` then set what you need.
- **Generic records** are generated for each use: `Page[CardListItem]` becomes `holder_card_list_item_page`.

Operations declared only for the daemon are outside the C API. When a native declaration uses a shape the generator cannot represent, generation must fail with a diagnostic. Extend the generator and its tests before exposing that shape; the declaration remains the source of the interface.
