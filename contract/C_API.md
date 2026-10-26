# The generated C API

`python -m holder_contract generate --output DIR` writes three files from the contract:

- **`holder_api.h`:** the C declarations consumers use: records, operations, error codes, and the arena.
- **`holder_api.hpp`:** the C++ declarations core implements, one function per operation, in `holder::api`. A missing implementation fails to link.
- **`holder_api_glue.cpp`:** the C entry points. Each checks its arguments against the contract, calls core's function, copies the result into the caller's arena, and turns error kinds into status codes. Core never sees a null pointer, an out-of-range number or an unknown enum value.

`tests/golden/` holds the output for today's contract, and `tests/c/` compiles and runs it against a stand-in for core.

## Conventions

- **Operations:**
  - Every operation is `int holder_<operation>(holder_context* context, <parameters>, holder_arena* arena, const <result>** out_<result>, holder_error** out_error)`.
  - It returns `HOLDER_OK` (0) or an error code, and sets `*out_error` as the rest of `holder.h` does (free it with `holder_error_destroy`).
  - An operation with no result has no `out_` parameter.
- **Error codes:** `HOLDER_ERROR_INVALID_ARGUMENT` (1) for anything the contract's checks reject, `HOLDER_ERROR_RUNTIME` (2) for an unexpected failure, and `HOLDER_ERROR_ALLOCATION` (3). Each error kind in the contract gets its own code from 100, in the contract's order, such as `HOLDER_ERROR_NOT_FOUND`.
- **Memory:** results live in the `holder_arena` the caller passes, until `holder_arena_free`. Nothing in a result needs freeing on its own.
- **Strings** are NUL-terminated UTF-8.
- **Nullable and optional values:**
  - A nullable or absent string is `NULL`.
  - A nullable, absent or defaulted number in a record is a pointer, `NULL` when absent.
- **Lists:** a list is a `{items, count}` struct.
- **Parameter defaults:**
  - A parameter whose default is the zero value (`false`, `0`) is passed by value, so zero means the default.
  - Any other defaulted number is a pointer, `NULL` for the default.
  - A defaulted string is `NULL` for the default.
- **Choices:**
  - A choice of literals is an enum. Its zero member is `…_DEFAULT` when the parameter has a default, and the other members start at 1.
  - A choice between literals and a string, such as `"roots"` or a card ID, is a `{kind, value}` struct whose zero kind means null.
  - Every enum ends with `…_MAX_ENUM = 0x7FFFFFFF`, so any `int` is a valid value of the type and the glue can reject a bad one without undefined behaviour.
- **Input records** are passed by pointer. A zeroed record means every optional field is absent: `holder_card_create card = {0};` then set what you need.
- **Generic records** are generated for each use: `Page[CardListItem]` becomes `holder_card_list_item_page`.

Daemon-only operations are left out. Parts of the vocabulary the C generator does not support yet stop generation with a message naming them: enums, host interfaces, error kinds with fields, maps, free-form JSON, secrets, streams, bytes, and fields that are both nullable and omittable. Each is added when the first contract declaration needs it.
