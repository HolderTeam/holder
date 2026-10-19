# Developing Holder for Python

Please refer to this package as Holder for Python or the Holder Python library,
or just Holder when the Python context is clear. Its scope is Holder
functionality using Python, independent of the current transport.

## Set up a checkout

Requires Python 3.11 or newer. From `python/holder/` in the Holder repository:

```sh
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -e '.[dev]'
python -m pytest
python scripts/generate.py --check
python -m build
```

The current implementation uses the daemon's HTTP API. The generated client
comes from [openapi-python-client](https://github.com/openapi-generators/openapi-python-client),
which is a development dependency. Installed clients need only HTTPX, attrs,
and typing-extensions. Entity and recovery policy belongs in the daemon or
core, not in this library.

## Layout and regeneration

```text
../../api/openapi.yaml  Owning HTTP contract for generation
openapi/                 Generator config and contract/tool provenance
src/holder/generated/    Generated transport, endpoint modules and models
src/holder/events.py     Handwritten incremental asyncio change-feed transport
scripts/generate.py      Regeneration and comparison against checked-in code
tests/                   Mock HTTP transport and contract coverage checks
```

Do not hand-edit `src/holder/generated`. It is isolated so it can be replaced
if a different implementation is needed. Handwritten transport and higher-level
APIs live beside it; the incremental change-feed transport is in
`src/holder/events.py`.

Fix contract errors in the canonical [`api/openapi.yaml`](../../../api/openapi.yaml)
and test them with the daemon before regenerating the client. The generator
finds that contract from its own location, regardless of the caller's working
directory.

```sh
# Regenerate from the owning contract.
python scripts/generate.py

# Compare the owning contract, provenance and generated output without changing files.
python scripts/generate.py --check
```

The generator and Ruff versions are pinned in `pyproject.toml`. Generation runs
in a temporary directory and fails on warnings before replacing the package.
The provenance file records the contract's SHA-256 and tool versions. A small
post-generation correction preserves the nullable push-phase status that
openapi-python-client 0.29.1 otherwise drops. The config treats YAML download
responses as plain text so their bodies are retained. CI checks the owning
contract against the generated client, runs tests and builds on Python 3.11
and 3.14. Installed packages contain the generated code and do not need the
repository contract.
