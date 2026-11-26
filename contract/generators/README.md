# Generators

Generators express [Holder's contract](../README.md) in the languages and transports its implementations and clients use. They own the translation; the declarations own the public interface.

## Responsibilities

The compiler owns source loading, declaration validation and the shared representation of types and operations. A generator owns target naming, type mapping, calling conventions, file layout and syntax. Target-specific templates belong beside their generator.

Each target exposes the operations available to it in the contract. Current application usage must not narrow a generated interface. A target must preserve the declared arguments, results, defaults and errors, and reject a shape it cannot support instead of silently omitting it.

Runtime bindings connect declared operations to implementation functions and services. Keep those bindings as explicit, validated data. Public interface decisions belong in the declarations; application behavior belongs behind the generated boundary. Adding an operation should not require a special case in the compiler.

Shared target conventions should be derived once and reused. For example, native clients and C glue must agree on the same C signatures and ownership rules. HTTP dispatch, codecs and OpenAPI must agree on the same request and response shapes.

## Working on a generator

Use a small declaration that exercises the feature, generate its output, and check that a consumer can use it. Compile and run generated native code where appropriate. Test failures as well as successful calls: rejecting unsupported declarations and invalid input is part of the interface.

Regenerate the repository outputs with `make -C contract update` and review them with the generator change. `make -C contract check` checks their freshness and runs the tooling tests. Generated artifacts live in their consuming projects and do not depend on the Python tooling at runtime.

## Implementation entry points

- [`core/`](core/): the native interface, C++ implementation declarations and C glue. [`abi.py`](core/abi.py) derives the C signatures used by native consumers.
- [`daemon/`](daemon/): HTTP dispatch, JSON codecs and OpenAPI. [`bindings.py`](daemon/bindings.py) connects daemon operations to runtime services; [`openapi.py`](daemon/openapi.py) adapts FastAPI's document to Holder's HTTP conventions.

  [`boundary.py`](daemon/boundary.py) validates the first complete daemon-only binding, including its generated types, handler signature and request/response boundary. [`inventory.py`](daemon/inventory.py) keeps remaining handwritten and partially generated boundaries explicit. See [HTTP.md](../HTTP.md#complete-daemon-only-boundary-proof) for the scope and validation evidence.
- [`android/`](android/): the Kotlin client and JNI bridge to core's C API.
- [`python/`](python/): shared Python Card records, typed operations and native error mappings.

Every generator consumes the typed compiler result described in [IR.md](../IR.md). The result includes a detached FastAPI document with additional response declarations, alongside typed HTTP metadata and named SSE event payload references. Backends do not import declarations or load a contract package. The daemon backend owns OpenAPI normalization, security merging, runtime service bindings and C++ event helper conventions.
