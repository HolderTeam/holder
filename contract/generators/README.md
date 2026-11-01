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

The existing generators still live in `holder_contract/` while they consume dictionary IR or depend on legacy helpers. Move a backend into `generators/core/`, `generators/daemon/`, or `generators/android/` when it consumes typed IR and has a clear target responsibility. This directory contains the boundary description until a backend is ready to move; generated artifacts continue to live in their target projects.
