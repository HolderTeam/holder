# Generators

Generators turn the compiler's canonical IR into files for a target. The flow is:

```text
interface/ declarations
    → Compiler.compile(ContractSource) → ContractView
    → core, daemon, and Android generators → generated source files
```

Each generator receives the same `ContractView`. The contract defines the full set of operations, arguments, result fields, and response options. A target selects operations by declared availability and transport: core exposes core operations, the daemon serves declared HTTP bindings and its own operations, and Android's JNI bridge exposes operations available through core. Current app usage does not narrow a generated interface.

The compiler owns source loading, validation, IR construction, and lowering that is independent of the output language. A generator owns target naming and type mapping, calling conventions, file layout, and syntax rendering. Jinja2 templates belong beside the generator that uses them. FastAPI remains the source of the daemon's OpenAPI document.

Target-specific runtime wiring, such as daemon service dependencies, is typed input data for a generator. Operation names do not select handwritten branches in generic compiler or generator code. Unsupported contract shapes should produce a clear error rather than silently omit part of the interface.

`common.py` holds what every generator shares: the `Unsupported` error and the generated-file header.

`core/types.py` lowers the contract's types for core: it resolves aliases, instantiates generic records, and names a record's choices of literals as enums. `core/abi.py` derives typed C signatures; the C header (`core/c_header.py`), the C++ declarations (`core/cpp_header.py`), the C glue (`core/glue.py`) and the Android JNI page renderer use those same signatures. `android/page.py` and its Jinja2 templates render the typed page operation.

`daemon/http.py` matches typed HTTP bindings to typed operations, locates each argument, and lowers its C++ declaration and body shape; `daemon/contract_routes.py` renders the contract's routes (ContractRoutes.h) from that plan, and `daemon/paths.py` turns route paths into regexes and rejects routes that would take the same request. `daemon/bindings.py` is the daemon's runtime wiring as typed data: the services each AI operation's handler receives, the AI exception classes and their statuses, and the records declared in `holder::api::routes`. `daemon/contract_json.py` generates the JSON codec for core's C++ types from the same lowering. `daemon/ai_routes.py` and `daemon/ai_json.py` generate the AI dispatch and its JSON codec from typed IR; the codec's records are those reachable from the operations with runtime bindings. Stream events are not in the IR yet, so it reads them from the authoring `Contract`.

OpenAPI adjustments and Android's per-feature bridges still live in `holder_contract/`. Move each into `core/`, `daemon/` or `android/` when it consumes typed IR and has a clear target responsibility. Generated artifacts continue to live in their target projects.
