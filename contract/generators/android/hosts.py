"""Kotlin host interfaces and JNI adapters for supported C callback tables."""

from __future__ import annotations

from compiler.ir import NullableType

from ..common import Unsupported
from ..core.abi import snake
from ..core.types import CoreTypes
from .client import CLASS_PATH, KOTLIN_DIR, TEMPLATES, camel, kdoc


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
