"""Kotlin host interfaces and JNI adapters for supported C callback tables."""

from __future__ import annotations

from compiler.ir import NullableType

from ..common import Unsupported
from ..core.abi import snake
from ..core.types import CoreTypes
from .client import CLASS_PATH, KOTLIN_DIR, TEMPLATES, camel, kdoc


def generate_android_hosts(types: CoreTypes) -> dict[str, str]:
    files = {}
    for op in types.host_operations:
        interface = types.contract.interface(op.host)
        methods = {method.name: method for method in interface.methods}
        # The pilot's supported shape is checked here as well as in CoreTypes: JNI needs an
        # explicit lowering for every method shape before another host interface can use it.
        if set(methods) != {"utc_offset", "current_zone"} or not isinstance(
            types.resolve(methods["utc_offset"].returns).type, NullableType
        ) or types.resolve(methods["current_zone"].returns).type.kind != "string":
            raise Unsupported(f"Android host adapter does not support {interface.name}")
        context = {"class_path": CLASS_PATH, "interface": interface, "operation": op.name,
                   "callback_type": f"holder_{snake(interface.name)}_callbacks",
                   "kotlin_operation": camel(op.name), "kdoc": kdoc}
        files[f"{KOTLIN_DIR}/{interface.name}.kt"] = TEMPLATES.get_template("host.kt.j2").render(**context)
        files[f"apps/mobile/android/app/src/main/cpp/generated/{snake(interface.name)}_host.cpp"] = \
            TEMPLATES.get_template("host.cpp.j2").render(**context)
    return files
