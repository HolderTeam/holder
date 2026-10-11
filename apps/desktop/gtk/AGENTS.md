# Coding-agent instructions

Treat the current code, tests, backend API contract, and explicitly referenced active task plans as authoritative. Other planning or design documents may be stale.

Holder Desktop is a thin GTK4/libadwaita client. Storage, indexing, search, Git operations, AI execution, and reusable domain logic belong in the backend/core rather than the frontend.

Prefer native GTK/libadwaita behaviour and preserve the project's supported runtime baseline.

Do not infer the current backend API from old planning documents. Use the current OpenAPI contract and implementation.

# The HTTP contract

The desktop is a client of the daemon like any third-party frontend, and is held to the same rule: it uses only the documented HTTP contract (`../../../api/openapi.yaml`). Do not depend on undocumented behaviour (for example how a field the contract does not describe as nullable happens to come back), and do not reach into the daemon's or core's code, files or databases. If the desktop needs something the contract does not offer, change the contract first, following "Changing the contract" in `../../../api/README.md`, and write any change in meaning into the contract's descriptions. `compatibility/daemon-api.json` records the range of `API_VERSION`s the desktop accepts.

Do not perform unrelated architectural cleanup while implementing a focused task.
