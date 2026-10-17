# Holder for Python

Holder is an open-source, local-first knowledge app for organising your ideas
and digital information as index cards and the connections between them.

Use Python to read and update the projects and cards in your running Holder
installation. Changes you make in a script appear in Holder too.

## Get started

1. You need the Holder application on your computer.
Get it from https://www.holder.team/

2. You need Python 3.11 or newer.
If in doubt, get the latest version (i.e. 3.15) from:
https://www.python.org/downloads/

3. Install the Holder Python library with:

```sh
python3 -m pip install holder-python
```

Save this as `list_cards.py` and run it with `python3 list_cards.py`:

```python
from holder import Holder

with Holder() as holder:
    for project in holder.projects:
        print(project.name)
        for card in project.cards:
            print("  ", card.title)
```

`Holder()` finds or starts your local Holder daemon using `holderctl`. The
`with` block closes the Python connection when the script ends; it does not
stop Holder.

## Create and edit cards

This example creates a project and two cards. Run it once: each run creates
another project named `Research`.

```python
from holder import Holder

with Holder() as holder:
    project = holder.projects.create(name="Research")
    card = project.cards.create(title="An idea", content="My notes")
    child = card.children.create(title="Next step")
    child.update(content="Try it out")
    card.tags.add("research")
```

To work with an existing project, find it by name and get a card by its ID:

```python
from holder import Holder

with Holder() as holder:
    matches = holder.projects.filter(name="Research")
    project = matches[0]
    for card in project.cards:
        print(card.id, card.title)

    # Replace this with an ID printed above.
    card = project.cards.get("card-id-here")
    card.update(title="A better title")
```

The name filter matches part of a name. If there are no matches, `[0]` raises
`IndexError`. Card content is available as `card.content`; use `card.update(...)`
to save edits. Card attributes cannot be assigned directly.

## Find cards

Search within a project and inspect the matching text:

```python
from holder import Holder

with Holder() as holder:
    project = holder.projects.filter(name="Research")[0]
    for result in project.cards.search("research notes"):
        print(result.title, result.snippet)
```

You can also list cards with a tag using
`project.cards.filter(tag="research")`, or list just top-level cards with
`project.cards.roots()`.

## More help

- [More scripting tasks](docs/scripting.md): moves, Trash, tags, collection
  refreshes, and connecting to a specific daemon.
- [Low-level client](docs/low-level-client.md): direct access to HTTP endpoints.
- [Development](docs/development.md): tests, builds, and generated code.
- [Publishing](docs/publishing.md): PyPI setup and release steps.

For offline analysis and bulk work without a running Holder installation, see
[Holder Kit](../kit/).
