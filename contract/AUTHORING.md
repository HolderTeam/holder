# Writing the contract

The [contract overview](README.md) explains its role and the development workflow. This guide describes the declaration vocabulary.

A contract package is a set of domain packages. Each has model modules for its records, error kinds and host interfaces, usually `models.py` and `errors.py`, and an `operations.py` for its operations. Errors every domain shares go in the package's own `errors.py`. `app.py` gives every error kind its status and code and assembles the domains in order, and `DECLARATION_ORDER` in `__init__.py` names every model module:

```python
"""Cards."""  # cards/models.py, a model module

from typing import Annotated

from authoring import Field, Record, Timestamp

type CardId = Annotated[str, Field(min_length=1)]
"""A card's ID."""


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
"""Errors every domain shares."""  # errors.py

from authoring import HolderError


class NotFound(HolderError):
    """The thing asked for does not exist, or is in the trash."""
```

```python
"""Errors particular to cards."""  # cards/errors.py

from authoring import HolderError


class InvalidMove(HolderError):
    """A card cannot go where it was asked to."""
```

```python
"""Card operations."""  # cards/operations.py

from typing import Annotated

from fastapi import Body, Path

from authoring import Contract

from ..errors import NotFound
from ..projects.ids import ProjectId
from .errors import InvalidMove
from .models import Card, CardId, CardMove

def register(contract: Contract) -> None:
    @contract.route("GET", "/projects/{project_id}/cards/{card_id}", errors=[NotFound])
    def get_card(project_id: Annotated[ProjectId, Path()], card_id: Annotated[CardId, Path()]) -> Card:
        """Fetch a card.

        A card in the trash or in another project is not found.
        """

    @contract.route("POST", "/projects/{project_id}/cards/{card_id}/move", errors=[NotFound, InvalidMove])
    def move_card(project_id: Annotated[ProjectId, Path()], card_id: Annotated[CardId, Path()],
                  move: Annotated[CardMove, Body()]) -> Card:
        """Move a card."""
```

```python
"""Assemble the public contract."""  # app.py

from authoring import Contract
from .cards import operations as card_operations
from .cards.errors import InvalidMove
from .errors import NotFound

contract = Contract(errors={NotFound: (404, 4040), InvalidMove: (422, 4220)})
card_operations.register(contract)
app = contract.app
```

```python
"""Holder's contract."""  # __init__.py

DECLARATION_ORDER = ("errors", "projects.ids", "cards.models", "cards.errors")
```

A model module may contain only:

- imports from `typing`, `enum`, `pydantic`, `authoring` and other contract modules;
- **records**: subclasses of `Record`, whose fields are documented by the string under each one. A record rejects fields it does not have, in requests and results alike (clients should still read results leniently), and a generic record's use is named for its arguments: `Page[CardListItem]` is `CardListItemPage`;
- **enums**: subclasses of `StrEnum`;
- **error kinds**: direct subclasses of `HolderError`, optionally with fields;
- **host interfaces**: `typing.Protocol` classes marked `@host_interface`, for callbacks core makes into the host, such as a keyring provider;
- **named types**: `type` statements, never plain assignments, so generators keep the name.
- **constants**: documented capitalised strings, `NAME: Final = "..."`, for text every platform must take from one place, such as Home's name and welcome card. They reach core's `Contract.h` and the Android client.

Anything else, such as a validator, a `model_config`, a function or any other assignment, is rejected before the file is imported, so a file that breaks the rules never runs.

`app.py` makes one `Contract` (`authoring.routes`) and calls each domain's `register(contract)` in public operation order. An operation module may import FastAPI's declaration helpers and define only route functions inside `register`; their bodies never run. `DECLARATION_ORDER` names every model module in the required IR order, so moving a module does not silently reorder declarations:

- **routes:** `@contract.route(method, path, errors=[...], status=..., core=...)` on a function whose body never runs. Each parameter says where it comes from, with FastAPI's `Path()`, `Query()` or `Body()`, which also carry its documentation (`Query(description=...)`). `errors` lists the error kinds it may fail with, besides an invalid argument; `status` is the success status, 200 unless given; `core=False` keeps an operation the daemon alone serves out of core's C API.
- **core-only operations:** `@contract.core_operation(errors=[...])`, for an operation only core's C API has; it is not a route.
- **runtime operations:** `@contract.runtime_operation(context=..., errors=[...])`, for one of core's operations that does not work through an open context: `context="none"` runs without one (its C function takes no context), `"opens"` makes one (it returns `None` in the declaration, and the context in C and Kotlin), and `"closes"` releases the one it is given (it takes nothing else and returns `None`). With `context="none"`, `error="reads"` reads an error core returned (it takes nothing else and returns `str`) and `error="closes"` releases it (returning `None`). They are not routes.
- **error statuses and codes:** `Contract(errors={NotFound: (404, 4040), ...})` gives each error kind its HTTP status and its C error code. Every error kind needs both. A code is four digits: its status, then one more digit to tell apart kinds that share the status (`KeyMissing` is `(409, 4091)`), so it reads as the HTTP status it echoes. Codes are unique, and are the error kinds' identity in the C API: once released, a code keeps its meaning and is never reused.

The extractor reads the routes FastAPI analysed, and the rest from the `Contract` ([HTTP.md](HTTP.md)). The IR records each operation's domain module as its source.

The rest of the vocabulary:

- **Sizes:** `Int32`, `Int64`, `Float64` and `Timestamp` (seconds since the Unix epoch). A bare `int` or `float` has no size in C and is rejected. A `Float64` is finite, since JSON has no NaN or infinity. Constrain one where it is used, as `Annotated[Int32, Field(ge=1, le=5000)]`; in JSON Schema each is inline, with its format (`int32`, `int64`, `double`).
- **Text:** a `str` is Unicode text without U+0000, so it is valid UTF-8 and fits in a C string. Lengths count characters.
- **Null and omitted:** `T | None` may be null. On a record's field, `T | MISSING = MISSING` may be omitted. Together, as in a partial update, they mean "omit to keep, null clears".
- **Other types:** `str`, `bool`, `list[T]`, `dict[str, T]`, `Literal[...]`, unions (discriminated with `Field(discriminator=...)`), `JsonValue` for free-form JSON, `SecretStr` for secrets, and generic records such as `class Page[T](Record)`.
- **Results only:** `Stream[T]` for an operation whose result is a sequence of items, and `Bytes` for one that returns a byte stream.
- **Constraints:** `Field(min_length=, max_length=, pattern=, ge=, le=, gt=, lt=)` in `Annotated`; a route's parameter takes them in its `Path()` or `Query()` too.

Every record, field, enum, error kind, interface method and operation must have documentation. An operation's docstring is its summary, then a blank line, then its description; nothing else is read from it.
