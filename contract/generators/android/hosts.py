"""Kotlin host interfaces and JNI adapters for supported C callback tables."""

from __future__ import annotations

from compiler.ir import LiteralType, NullableType

from ..common import Unsupported
from ..core.abi import literal_member, snake
from ..core.types import CoreTypes
from .client import CLASS_PATH, KOTLIN_DIR, TEMPLATES, camel, kdoc, pascal


def generate_android_hosts(types: CoreTypes) -> dict[str, str]:
    def shape(ref) -> str:
        resolved = types.resolve(ref).type
        return f"nullable {shape(resolved.value)}" if isinstance(resolved, NullableType) else resolved.kind

    def method_shape(method) -> tuple[tuple[str, ...], str]:
        return tuple(shape(parameter.type) for parameter in method.parameters), shape(method.returns)

    files = {}
    for op in types.host_operations:
        interface = types.contract.interface(op.host)
        methods = {method.name: method for method in interface.methods}
        keyring_shapes = {
            "lookup": (("int32", "string", "string", "nullable string"), "nullable string"),
            "store": (("int32", "string", "string", "nullable string", "string", "string"), "none"),
            "remove": (("int32", "string", "string", "nullable string"), "none"),
        }
        if interface.name == "KeyringProvider":
            if {name: method_shape(method) for name, method in methods.items()} != keyring_shapes:
                raise Unsupported("Android host adapter does not support KeyringProvider's callback shapes")
            files[f"{KOTLIN_DIR}/KeyringProvider.kt"] = TEMPLATES.get_template("keyring.kt.j2").render()
            files["apps/mobile/android/app/src/main/cpp/generated/keyring_provider_host.cpp"] = \
                TEMPLATES.get_template("keyring.cpp.j2").render()
            continue
        if interface.name == "SshSigner":
            if {name: method_shape(method) for name, method in methods.items()} != {
                "sign": (("bytes",), "bytes")
            } or tuple(shape(parameter.type) for parameter in op.parameters) != ("string", "bytes") or \
                    op.context != "required":
                raise Unsupported("Android host adapter does not support SshSigner's callback shapes")
            files[f"{KOTLIN_DIR}/SshSigner.kt"] = TEMPLATES.get_template("signer.kt.j2").render()
            files["apps/mobile/android/app/src/main/cpp/generated/ssh_signer_host.cpp"] = \
                TEMPLATES.get_template("signer.cpp.j2").render()
            continue
        if interface.name == "StorageProvider":
            expected = {
                "put": (("string", "string", "int64", "string"), "none"),
                "get": (("string", "string"), "none"),
                "exists": (("string",), "bool"),
                "remove": (("string",), "none"),
            }
            if {name: method_shape(method) for name, method in methods.items()} != expected or \
                    tuple(shape(parameter.type) for parameter in op.parameters) != ("literal",) or \
                    op.context != "none" or not interface.error_codes:
                raise Unsupported("Android host adapter does not support StorageProvider's callback shapes")
            parameter = op.parameters[0]
            literal = types.resolve(parameter.type).type
            assert isinstance(literal, LiteralType)
            kind = {"name": parameter.name, "kotlin": pascal(op.name) + pascal(parameter.name),
                    "c_type": f"holder_{op.name}_{parameter.name}",
                    "members": [(literal_member(value), str(value)) for value in literal.values]}
            files[f"{KOTLIN_DIR}/StorageProvider.kt"] = TEMPLATES.get_template("storage.kt.j2").render(
                error_codes=interface.error_codes, kind=kind)
            files["apps/mobile/android/app/src/main/cpp/generated/storage_provider_host.cpp"] = \
                TEMPLATES.get_template("storage.cpp.j2").render(kind=kind)
            continue
        # The pilot's supported shape is checked here as well as in CoreTypes: JNI needs an
        # explicit lowering for every method shape before another host interface can use it.
        if interface.name != "TimeZoneProvider" or {
            name: method_shape(method) for name, method in methods.items()
        } != {"utc_offset": (("string", "int64"), "nullable int64"),
              "current_zone": ((), "string")}:
            raise Unsupported(f"Android host adapter does not support {interface.name}")
        context = {"class_path": CLASS_PATH, "interface": interface, "operation": op.name,
                   "callback_type": f"holder_{snake(interface.name)}_callbacks",
                   "kotlin_operation": camel(op.name), "kdoc": kdoc}
        files[f"{KOTLIN_DIR}/{interface.name}.kt"] = TEMPLATES.get_template("host.kt.j2").render(**context)
        files[f"apps/mobile/android/app/src/main/cpp/generated/{snake(interface.name)}_host.cpp"] = \
            TEMPLATES.get_template("host.cpp.j2").render(**context)
    return files
