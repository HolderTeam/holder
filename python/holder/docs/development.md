# Developing Holder for Python

Please refer to this package as Holder for Python or the Holder Python library,
or just Holder when the Python context is clear. Its scope is Holder
functionality using Python, independent of the current transport.

## Set up a checkout

Requires Python 3.11 or newer. From `python/holder/` in the Holder repository:

```sh
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -e '.[dev]' -r requirements-generator.txt
python -m pytest
python scripts/generate.py --check
python -m build
```

The current implementation uses the daemon's HTTP API. The generated client
comes from a fork of
[openapi-python-client](https://github.com/openapi-generators/openapi-python-client)
that reads OpenAPI 3.2 and generates streaming responses
([zeth/openapi-python-client](https://github.com/zeth/openapi-python-client),
branch `openapi-3.2`). PyPI rejects packages whose dependencies are Git URLs, so
the fork is pinned to a commit in `requirements-generator.txt` rather than in
the `dev` extra. Installed clients need only HTTPX, attrs, and
typing-extensions. Entity and recovery policy belongs in the daemon or
core, not in this library.

## Layout and regeneration

```text
../../api/openapi.yaml  Owning HTTP contract for generation
openapi/                 Generator config and contract/tool provenance
src/holder/generated/    Generated transport, endpoint modules, models and streams
src/holder/events.py     Change-feed cursor and acknowledgement over the generated stream
scripts/generate.py      Regeneration and comparison against checked-in code
tests/                   Mock HTTP transport and contract coverage checks
```

Do not hand-edit `src/holder/generated`. It is isolated so it can be replaced
if a different implementation is needed. Higher-level APIs live beside it.

Event streams (`text/event-stream`) and binary downloads are generated too:
each such endpoint has `stream()` and `astream()`, which read the response as
it arrives. Event data is decoded into the model the contract declares for
that event name. A stream that follows a GET reconnects after a dropped
connection, sending `Last-Event-ID`, and stops when the daemon answers 204.
A refused stream raises the generated `StreamError`, which the library maps to
its own errors in `src/holder/_streams.py`.

Fix contract errors in the daemon's declarations, which generate the canonical
[`api/openapi.yaml`](../../../api/openapi.yaml) (see
[Changing the contract](../../../api/README.md#changing-the-contract)), and test
them with the daemon before regenerating the client. The generator
finds that contract from its own location, regardless of the caller's working
directory.

```sh
# Regenerate from the owning contract.
python scripts/generate.py

# Compare the owning contract, provenance and generated output without changing files.
python scripts/generate.py --check
```

Ruff is pinned in `pyproject.toml` and the generator in
`requirements-generator.txt`; `scripts/generate.py` names both and refuses to
regenerate with anything else installed. Generation runs in a
temporary directory and fails on warnings before replacing the package. The
provenance file records the contract's SHA-256, the generator's repository and
commit, and the Ruff version. The config treats YAML download
responses as plain text so their bodies are retained, and a response of any
media type (`*/*`, the documentation's files) as binary. CI checks the owning
contract against the generated client, runs tests and builds on Python 3.11
and 3.14. Installed packages contain the generated code and do not need the
repository contract.
