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

The HTTP backend uses the daemon's API. Its generated client comes from a fork of
[openapi-python-client](https://github.com/openapi-generators/openapi-python-client)
that reads OpenAPI 3.2 and generates streaming responses
([zeth/openapi-python-client](https://github.com/zeth/openapi-python-client),
branch `openapi-3.2`). PyPI rejects packages whose dependencies are Git URLs, so
the fork is pinned to a commit in `requirements-generator.txt` rather than in
the `dev` extra. Installed clients need only HTTPX, attrs, and
typing-extensions. Entity and recovery policy belongs in the daemon or
core, not in this library.

## Build the Linux wheel variants

The default build produces a universal HTTP wheel. To build a native wheel
with the same version, stage Kit against a core SDK built from the same
checkout. Run the following from the repository root with an absolute SDK
path in `HOLDER_CORE_SDK`:

```sh
work=$(mktemp -d)
cmake -S python/kit -B "$work/kit" -DCMAKE_INSTALL_PREFIX="$work/stage"
cmake --build "$work/kit"
cmake --install "$work/kit"
python -m build --wheel --outdir "$work/wheels" python/holder
HOLDER_PYTHON_EMBEDDED_STAGE="$work/stage" \
  python -m build --wheel --outdir "$work/wheels" python/holder
```

CMake and the wheel build must use the same CPython interpreter. Set
`-DPython_EXECUTABLE=/path/to/python` on the CMake command if needed. The
native build packages the staged extension and schema under `holder`.
A compatible Linux CPython prefers that wheel when both variants are offered
at the same version. The native wheel from this procedure uses the build
machine's system libraries. Preparing redistributable wheels and integrating
them into releases is separate work.

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
