# The generated C API

`make -C contract update` writes three files from the contract into core, and `make -C contract check` fails when they are out of date:

- **`core/include/holder/holder_api.h`:** the C declarations consumers use: records, operations, error codes, and the arena. `holder.h` includes it, so consumers still include only `holder.h`.
- **`core/src/api/Contract.h`:** the C++ declarations core implements, one function per operation, in `holder::contract`, a namespace of its own so nothing clashes with the daemon's `holder::api`. A missing implementation fails to link.
- **`core/src/api/ContractCApi.cpp`:** the C entry points. Each checks its arguments against the contract, calls core's function, copies the result into the caller's arena, and turns error kinds into status codes. Core never sees a null pointer, an out-of-range number or an unknown enum value.

Core implements the operations by hand in `core/src/api/` (`Cards.cpp`). A record's C++ struct inherits the record it extends, so `CardMetadata` is one type; core's `holder::model::Card` is an alias of it. In C++, 64-bit integers are `long long`, as in the rest of core; the glue converts them to `int64_t` at the C boundary. `tests/c/` compiles the generated files against a stand-in for core and runs them.

## Conventions

- **Operations:**
  - Every operation is `int holder_<operation>(holder_context* context, <parameters>, holder_arena* arena, const <result>** out_<result>, holder_error** out_error)`.
  - It returns `HOLDER_OK` (0) or an error code, and sets `*out_error` as the rest of `holder.h` does (free it with `holder_error_destroy`).
  - An operation with no result has no `out_` parameter.
  - An operation that returns a list returns it as a `{items, count}` struct, in `out_<record>_list`, such as `const holder_tag_count_list** out_tag_count_list`.
  - An operation on a card takes the card's `project_id` before its `card_id`, and does not find a card in another project.
- **Error codes:** `HOLDER_ERROR_INVALID_ARGUMENT` (1) for anything the contract's checks reject, `HOLDER_ERROR_RUNTIME` (2) for an unexpected failure, and `HOLDER_ERROR_ALLOCATION` (3). Each error kind in the contract gets its own code from 100, in the contract's order, such as `HOLDER_ERROR_NOT_FOUND`.
- **Memory:** results live in the `holder_arena` the caller passes, until `holder_arena_free`. Nothing in a result needs freeing on its own.
- **Strings** are NUL-terminated UTF-8. The glue rejects text that is not well-formed UTF-8, and length constraints count characters, not bytes.
- **Numbers:** the glue rejects a `double` that is NaN or infinite.
- **Nullable and optional values:**
  - A nullable or absent string is `NULL`.
  - A nullable, absent or defaulted number in a record is a pointer, `NULL` when absent.
- **Partial updates:** a field of an input record that is both nullable and omittable, such as a card's new parent in an update, is a `holder_string_change`: `{change, value}`, where `change` is `HOLDER_KEEP` (zero, so a zeroed record changes nothing), `HOLDER_SET` (to `value`) or `HOLDER_CLEAR` (to null). In C++ it is a `Change<T>`, `std::optional<std::optional<T>>`: `nullopt` keeps the field, an empty value clears it. Only strings have them so far, and results never do.
- **Lists:** a list is a `{items, count}` struct.
- **Parameter defaults:**
  - A parameter whose default is the zero value (`false`, `0`) is passed by value, so zero means the default.
  - Any other defaulted number is a pointer, `NULL` for the default.
  - A defaulted string is `NULL` for the default.
- **Choices:**
  - A choice of literals is an enum. Its zero member is `…_DEFAULT` when the parameter has a default, and the other members start at 1.
  - A choice between literals and a string, such as `"roots"` or a card ID, is a `{kind, value}` struct whose zero kind means null.
  - A required choice of strings in an input record is an enum named for the record and the field, such as `holder_card_move_intent`, whose members start at 1; the glue rejects zero. In C++ it is an `enum class` (`CardMoveIntent`). Optional choices, and choices in results, are not supported yet.
  - Every enum ends with `…_MAX_ENUM = 0x7FFFFFFF`, so any `int` is a valid value of the type and the glue can reject a bad one without undefined behaviour.
- **Input records** are passed by pointer. A zeroed record means every optional field is absent: `holder_new_card card = {0};` then set what you need.
- **Generic records** are generated for each use: `Page[CardListItem]` becomes `holder_card_list_item_page`.

Daemon-only operations are left out. Parts of the vocabulary the C generator does not support yet stop generation with a message naming them: enums, host interfaces, error kinds with fields, maps, free-form JSON, secrets, streams, bytes, partial updates of anything but a string, and constraints the glue cannot check yet (`pattern`, and bounds on a `float64`). Each is added when the first contract declaration needs it.
