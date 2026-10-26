# contract

Holder's interface, written once. Nothing in this directory runs in Holder.

These files describe every record, operation, error and event that core, the daemon and the clients share. A generator turns them into code that does run: `holder.h`, the C++ declarations core implements, the daemon's routes and OpenAPI, and the client libraries. To change Holder's interface, edit these files; to change what it does, edit the code that implements it.

The files look like a FastAPI app, but they are declarations, not a server. An operation is a function with a typed signature and a docstring, and no body; nothing ever calls it.

The plan is tracked in [#286](https://github.com/HolderTeam/holder/issues/286). Today the contract declares the Card slice ([#289](https://github.com/HolderTeam/holder/issues/289)): getting, listing and creating cards. The C generator writes core's C API for it, described in [C_API.md](C_API.md), and core implements it in `core/src/api/`.

## Layout

- `interface/` is the contract: one module per area of Holder's interface.
- `holder_contract/` is the tooling:
  - the vocabulary contract files import (`vocabulary.py`) and the rules they must follow (`rules.py`);
  - the extractor that reads them into an intermediate representation, the IR (`extract.py`, described in [IR.md](IR.md));
  - the generators (`generate.py`, for core's C API);
  - Hypothesis strategies for the values of every type in the IR, and a check that a value conforms to its type (`values.py`).
- `tests/` tests the tooling: the extractor against a sample contract in `tests/fixtures/sample/`, which uses every part of the vocabulary; the generator's output by compiling and running it against a stand-in for core (`tests/c/`); and the strategies against the conformance check and Pydantic. Core's own tests (`core/tests/CApiCards_test.cpp`) run the real implementation.
- `tests/conformance/` tests core against the contract: Hypothesis calls the operations through the generated C API of a shared `libholder`, with arguments from the strategies, and checks each result against its declared type and against a model of what it should be. Core CI runs them on Linux.

None of this is built, packaged or published. `pyproject.toml` holds only tool settings, and CI fails if anything outside this directory imports `holder_contract` or depends on it ([.github/scripts/check-contract-boundary.py](../.github/scripts/check-contract-boundary.py)). The generated Python client has its own models, even where they look like the contract's records.

## Working on it

Python 3.12 or later, with the pinned tools:

```sh
python3 -m pip install -r contract/requirements.txt
make -C contract check    # lint, test, and check the contract
make -C contract ir       # write the IR to contract/build/ir.json
make -C contract update   # after changing the contract or the generator: rewrite core's generated files
```

`check` fails when core's generated files differ from what the contract produces, or when a generated name is already used in the hand-written part of `holder.h`. Commit the contract change and the regenerated files together.

`python -m holder_contract check` reports every problem in the contract at once, each with its file and line.

The conformance tests are skipped unless `HOLDER_LIBRARY` names a shared `libholder` built from the same commit:

```sh
cmake -S core -B core/build -DHOLDER_CORE_BUILD_SHARED=ON && cmake --build core/build
HOLDER_LIBRARY=$PWD/core/build/libholder.so python3 -m pytest contract/tests/conformance
```

`HOLDER_CONFORMANCE_EXAMPLES` sets how many runs Hypothesis makes; the default is 25, and CI uses 100. A new operation needs rules in a conformance test, so that the model says what it should do.

## Writing the contract

```python
"""Cards."""

from typing import Annotated

from holder_contract import Field, HolderError, Record, Timestamp, http, operation

type CardId = Annotated[str, Field(min_length=1)]
"""A card's ID."""


class NotFound(HolderError):
    """The thing asked for does not exist, or is in the trash."""


class Card(Record):
    """A card in a project."""

    card_id: CardId
    """The card's ID."""
    parent_card_id: CardId | None
    """Null for a root card."""
    deleted_at: Timestamp | None
    """When the card was moved to the trash; null when it is not in the trash."""


@operation(errors=[NotFound])
def get_card(card_id: CardId) -> Card:
    """Fetch a card.

    A card in the trash is not found.
    """


http(get_card, "GET", "/cards/{card_id}")
```

A file may contain only:

- imports from `typing`, `enum`, `pydantic`, `holder_contract` and other contract modules;
- **records**: subclasses of `Record`, whose fields are documented by the string under each one;
- **enums**: subclasses of `StrEnum`;
- **error kinds**: direct subclasses of `HolderError`, optionally with fields;
- **host interfaces**: `typing.Protocol` classes marked `@host_interface`, for callbacks core makes into the host, such as a keyring provider;
- **operations**: functions marked `@operation`, whose body is only a docstring or `...`;
- **named types**: `type` statements, never plain assignments, so generators keep the name;
- **HTTP bindings**: `http(operation, method, path, query=[...])` calls, which core ignores.

Anything else, such as a validator, a `model_config`, a helper function or an assignment, is rejected before the file is imported, so a file that breaks the rules never runs.

The rest of the vocabulary:

- **Sizes:** `Int32`, `Int64`, `Float64` and `Timestamp` (seconds since the Unix epoch). A bare `int` or `float` has no size in C and is rejected. A `Float64` is finite, since JSON has no NaN or infinity.
- **Text:** a `str` is Unicode text without U+0000, so it is valid UTF-8 and fits in a C string. Lengths count characters.
- **Null and omitted:** `T | None` may be null. On a record's field, `T | MISSING = MISSING` may be omitted. Together, as in a partial update, they mean "omit to keep, null clears".
- **Other types:** `str`, `bool`, `list[T]`, `dict[str, T]`, `Literal[...]`, unions (discriminated with `Field(discriminator=...)`), `JsonValue` for free-form JSON, `SecretStr` for secrets, and generic records such as `class Page[T](Record)`.
- **Results only:** `Stream[T]` for an operation whose result is a sequence of items, and `Bytes` for one that returns a byte stream.
- **Constraints:** `Field(min_length=, max_length=, pattern=, ge=, le=, gt=, lt=)` in `Annotated`, and `Field(description=...)` for a parameter's documentation.
- **Operation options:** `@operation(errors=[...])` lists the error kinds an operation may fail with, besides an invalid argument. `daemon_only=True` keeps it out of core's C API.

Every record, field, enum, error kind, interface method and operation must have documentation. An operation's docstring is its summary, then a blank line, then its description; nothing else is read from it.
