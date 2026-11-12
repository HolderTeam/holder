# Holder's contract

Holder's public programming interface is defined in Python, using Pydantic models and FastAPI declarations. The contract describes the operations callers can use, the data they exchange, the errors they can encounter, and the meaning of those operations.

The C API and OpenAPI specification are generated expressions of that contract. Embedding Holder and talking to a running daemon are different ways to use the same system. Their interfaces should agree wherever they expose the same operation.

## What the contract owns

The declarations in [`interface/`](interface/) are the source of truth for public operations and types. They bring together signatures, constraints, defaults, errors, and documentation. Operations declare where they are available: some belong to core, some to the daemon, and some are exposed through both.

Pydantic supplies the language for data shapes and constraints. FastAPI supplies the language for HTTP bindings and produces the basis of the OpenAPI document. The contract also describes what native callers need, including sized numbers, context lifetimes and host callbacks.

Descriptions are part of the contract. A schema can say that an operation accepts a card ID; its description says what happens if that card is in the trash. Such rules belong beside the declaration, with behavioral tests that check the implementation keeps the promise.

## From declaration to implementation

```text
Python contract
    ├── C API → applications embedding core
    ├── OpenAPI → HTTP clients and client libraries
    ├── Kotlin client and JNI bridge → Android using core's C API
    └── generated types, validation and adapters → core and daemon implementations
```

The compiler checks declarations and gives generators a shared representation of them. Generators translate the interface into the conventions of each target: C calling conventions and memory ownership, HTTP requests and responses, or a client language's types.

Android's Kotlin client and JNI bridge are generated from the same contract and C signatures as core's C glue, so its native interface follows the declarations through to the application.

Core and the daemon implement the behavior behind those interfaces. Core owns reusable domain behavior; the daemon supplies the running service and its HTTP transport. Clients use the public interface appropriate to their environment. The Python contract tooling runs during development and generation; deployed Holder components use its outputs.

The design goal is that each public interface decision is made once. A field, default, error or response shape should flow from its declaration to every affected boundary. Handwritten adapters still exist; as generation expands, their interface mechanics belong in generators, while their application behavior stays in the implementation. This is the direction for new work as well as the standard for reviewing existing code.

## Changing Holder

Start an interface change in the contract. Describe the operation and its meaning, regenerate the affected interfaces, then implement and test the behavior. Include the declarations, generated files and affected consumers in the same change. A change entirely behind an existing interface can stay in the implementation.

Generated files are committed so they can be reviewed and built without the contract tooling. Edit their source declarations or generators, then regenerate. When a target cannot express a declaration, extend its generator or revise the design explicitly; generation must not silently drop part of the contract.

Review both shape and meaning. Generated-file checks detect drift. Compilation checks that implementations and consumers fit the generated interfaces. Conformance tests exercise the real C and HTTP interfaces against declared types and behavioral expectations. Compatibility includes changes in meaning even when signatures stay the same; see the [HTTP compatibility policy](../api/README.md#changing-the-contract) and [core's consumption model](../core/README.md#consumption-model).

## Working on the contract

Use Python 3.12 or later and the pinned dependencies. Core's generated C and C++ files are laid out
by `clang-format-18` with core's `.clang-format`, so generating or checking them also needs that
exact version on `PATH` (another version lays code out differently, and `check` would report the
files out of date):

```sh
python3 -m pip install -r contract/requirements.txt
sudo apt install clang-format-18   # or your platform's package
make -C contract update   # regenerate interfaces
make -C contract check    # lint, tests, declaration checks and generated-file freshness
```

The Python HTTP client has a separate generation step described in its [development guide](../python/holder/docs/development.md).

To exercise real implementations, set `HOLDER_LIBRARY` to a shared `libholder` and/or `HOLDER_DAEMON` to a `holderd`, built from the same checkout, then run `python3 -m pytest contract/tests/conformance`. Without those settings, the corresponding integration tests are skipped.

## Further reading

- [Writing the contract](AUTHORING.md): declaration examples and vocabulary.
- [C API conventions](C_API.md) and [HTTP conventions](HTTP.md): how the contract is expressed to callers.
- [Compiler representation](IR.md) and [generator responsibilities](generators/README.md): working on the tooling.

The directory follows those responsibilities: `interface/` holds Holder's declarations, `authoring/` supplies their vocabulary, `compiler/` checks and extracts them, and `generators/` produces target code. `tests/` and `test_support/` verify the tooling and the implementations.
