# contract

Holder's interface, written once. Nothing in this directory runs in Holder.

These files describe every record, operation, error and event that core, the daemon and the clients share. A generator turns them into code that does run: `holder.h`, the C++ declarations core implements, the daemon's routes and OpenAPI, and the client libraries. To change Holder's interface, edit these files; to change what it does, edit the code that implements it.

The operations are a FastAPI app, but a declaration, not a server: each is a route with a typed signature and a docstring, and a body nothing ever calls. The daemon serves the routes from code generated from the contract.

The plan is tracked in [#286](https://github.com/HolderTeam/holder/issues/286). Today the contract declares the Card slice ([#289](https://github.com/HolderTeam/holder/issues/289)): getting, listing, creating, changing, moving, trashing and restoring cards. The C generator writes core's C API for it, described in [C_API.md](C_API.md), and core implements it in `core/src/api/`. The daemon's generators write its HTTP routes, their JSON codec and their OpenAPI paths, described in [HTTP.md](HTTP.md).

## Layout

- `interface/` is the contract. `cards/` and `projects/` keep their models and operations together; `projects/ids.py` owns the Project ID shared by other domains. Other domains are moving from root model modules and `operations/` modules to the same layout. `app.py` assembles operations, and `DECLARATION_ORDER` in `__init__.py` fixes model ordering independently of file paths.
- `authoring/` is the Pydantic and FastAPI vocabulary imported by the contract: records, native types, errors, host interfaces and route declarations.
- `compiler/` holds declaration rules, source loading, the target-independent Pydantic IR, and `Compiler.compile(ContractSource)`, the typed entry point. Its builder produces typed IR objects.
- `generators/` describes the backend boundary in its [README](generators/README.md). It contains the typed C ABI plan shared by core and Android, and Android's generated page operation renderer.
- `holder_contract/` holds the current CLI and dictionary adapter for generators awaiting migration: core's C API, daemon routes and JSON codec, OpenAPI, Android's Kotlin/JNI bridges, and their Jinja2 templates. It also holds the Hypothesis value strategies used by tests.
- `tests/` tests the tooling: the extractor against a sample contract in `tests/fixtures/sample/`, which uses every part of the vocabulary; the generators' output by compiling and running it against a stand-in for core (`tests/c/` for the C API, `tests/http/` for the routes); and the strategies against the conformance check and Pydantic. Core's own tests (`core/tests/CApiCards_test.cpp`) run the real implementation.
- `tests/conformance/` tests Holder against the contract: Hypothesis calls the operations, with arguments from the strategies, through the generated C API of a shared `libholder` and over HTTP against a `holderd`, and checks each result against its declared type and against a model of what it should be. Over HTTP, every response must also be one its route declares: a status it lists, and a body its Pydantic model accepts. Schemathesis (`test_schemathesis.py`) also sends the daemon requests valid and invalid by `api/openapi.json`, for the contract's routes, and checks each answer against the document. Core CI runs them through the C API on Linux, and Daemon CI over HTTP.

None of this is built, packaged or published. `pyproject.toml` holds only tool settings, and CI checks that code outside this directory does not depend on the contract tooling ([.github/scripts/check-contract-boundary.py](../.github/scripts/check-contract-boundary.py)). The generated Python client has its own models, even where they look like the contract's records.

## Working on it

Python 3.12 or later, with the pinned tools:

```sh
python3 -m pip install -r contract/requirements.txt
make -C contract check    # lint, test, and check the contract
make -C contract ir       # write the IR to contract/build/ir.json
make -C contract update   # after changing the contract or a generator: rewrite the generated files in core, Android, daemon/ and api/
```

`check` fails when a generated file in core, Android, the daemon or `api/` differs from what the contract produces, or when a generated name is already used in the hand-written part of `holder.h`. Commit the contract change and the regenerated files together.

`python -m holder_contract check` reports every problem in the contract at once, each with its file and line.

The conformance tests run through the C API when `HOLDER_LIBRARY` names a shared `libholder`, and over HTTP when `HOLDER_DAEMON` names a `holderd`, each built from the same commit; without them they are skipped:

```sh
cmake -S core -B core/build -DHOLDER_CORE_BUILD_SHARED=ON && cmake --build core/build
daemon/make.sh build
HOLDER_LIBRARY=$PWD/core/build/libholder.so HOLDER_DAEMON=$PWD/daemon/build/holderd \
  python3 -m pytest contract/tests/conformance
```

`HOLDER_CONFORMANCE_EXAMPLES` sets how many runs Hypothesis makes; the default is 25, and CI uses 100. A new operation needs rules in a conformance test, so that the model says what it should do. Schemathesis covers a route once it is in the contract; a rule the document cannot state, such as which cursors are valid, goes in `stated_in_words` there.

## Writing the contract

A contract package has model modules for records, error kinds and host interfaces. A domain package can place its operations beside its models; `app.py` assembles domains in order:

```python
"""Cards."""  # cards/models.py, a model module

from typing import Annotated

from authoring import Field, HolderError, Record, Timestamp

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
```

```python
"""Card operations."""  # cards/operations.py

from typing import Annotated

from fastapi import Path

from authoring import Contract

from .models import Card, CardId
from ..projects.ids import ProjectId
from ..errors import NotFound

def register(contract: Contract) -> None:
    @contract.route("GET", "/projects/{project_id}/cards/{card_id}", errors=[NotFound])
    def get_card(project_id: Annotated[ProjectId, Path()], card_id: Annotated[CardId, Path()]) -> Card:
        """Fetch a card.

        A card in the trash or in another project is not found.
        """
```

```python
"""Assemble the public contract."""  # app.py

from authoring import Contract
from .errors import NotFound
from .cards import operations as card_operations

contract = Contract(statuses={NotFound: 404})
card_operations.register(contract)
app = contract.app
```

A model module may contain only:

- imports from `typing`, `enum`, `pydantic`, `authoring` and other contract modules;
- **records**: subclasses of `Record`, whose fields are documented by the string under each one. A record rejects fields it does not have, in requests and results alike (clients should still read results leniently), and a generic record's use is named for its arguments: `Page[CardListItem]` is `CardListItemPage`;
- **enums**: subclasses of `StrEnum`;
- **error kinds**: direct subclasses of `HolderError`, optionally with fields;
- **host interfaces**: `typing.Protocol` classes marked `@host_interface`, for callbacks core makes into the host, such as a keyring provider;
- **named types**: `type` statements, never plain assignments, so generators keep the name.

Anything else, such as a validator, a `model_config`, a function or an assignment, is rejected before the file is imported, so a file that breaks the rules never runs.

`app.py` makes one `Contract` (`authoring.routes`) and calls each domain's `register(contract)` in public operation order. An operation module may import FastAPI's declaration helpers and define only route functions inside `register`; their bodies never run. `DECLARATION_ORDER` names every model module in the required IR order, so moving a module does not silently reorder declarations:

- **routes:** `@contract.route(method, path, errors=[...], status=..., core=...)` on a function whose body never runs. Each parameter says where it comes from, with FastAPI's `Path()`, `Query()` or `Body()`, which also carry its documentation (`Query(description=...)`). `errors` lists the error kinds it may fail with, besides an invalid argument; `status` is the success status, 200 unless given; `core=False` keeps an operation the daemon alone serves out of core's C API.
- **core-only operations:** `@contract.core_operation(errors=[...])`, for an operation only core's C API has; it is not a route.
- **error statuses:** `Contract(statuses={NotFound: 404, ...})` gives each error kind its HTTP status. Every error kind of a route needs one.

The extractor reads the routes FastAPI analysed, and the rest from the `Contract` ([HTTP.md](HTTP.md)). The IR records each operation's domain module as its source.

The rest of the vocabulary:

- **Sizes:** `Int32`, `Int64`, `Float64` and `Timestamp` (seconds since the Unix epoch). A bare `int` or `float` has no size in C and is rejected. A `Float64` is finite, since JSON has no NaN or infinity. Constrain one where it is used, as `Annotated[Int32, Field(ge=1, le=5000)]`; in JSON Schema each is inline, with its format (`int32`, `int64`, `double`).
- **Text:** a `str` is Unicode text without U+0000, so it is valid UTF-8 and fits in a C string. Lengths count characters.
- **Null and omitted:** `T | None` may be null. On a record's field, `T | MISSING = MISSING` may be omitted. Together, as in a partial update, they mean "omit to keep, null clears".
- **Other types:** `str`, `bool`, `list[T]`, `dict[str, T]`, `Literal[...]`, unions (discriminated with `Field(discriminator=...)`), `JsonValue` for free-form JSON, `SecretStr` for secrets, and generic records such as `class Page[T](Record)`.
- **Results only:** `Stream[T]` for an operation whose result is a sequence of items, and `Bytes` for one that returns a byte stream.
- **Constraints:** `Field(min_length=, max_length=, pattern=, ge=, le=, gt=, lt=)` in `Annotated`; a route's parameter takes them in its `Path()` or `Query()` too.

Every record, field, enum, error kind, interface method and operation must have documentation. An operation's docstring is its summary, then a blank line, then its description; nothing else is read from it.
