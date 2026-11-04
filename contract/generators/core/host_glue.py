"""C callback tables and ownership adapters for contract host interfaces."""

from __future__ import annotations

from compiler.ir import NullableType

from ..common import contract_sources, header_lines
from .abi import snake
from .types import CoreTypes


def generate_host_glue(types: CoreTypes) -> str:
    out = [*header_lines(contract_sources(types.contract),
                         "The C callback adapters for host-implemented contract interfaces.", "cpp"),
           '#include "Contract.h"', '#include "holder/holder_api.h"', "", "#include <cstdint>",
           "#include <cstdlib>", "#include <memory>", "#include <new>", "#include <optional>",
           "#include <stdexcept>", "#include <string>", "", "namespace {", ""]
    for op in types.host_operations:
        interface = types.contract.interface(op.host)
        callbacks = f"holder_{snake(interface.name)}_callbacks"
        adapter = f"C{interface.name}Callbacks"
        out += [f"class {adapter} final : public holder::contract::{interface.name} {{", " public:",
                f"  {adapter}({callbacks} callbacks, void* user_data, holder_destroy_fn destroy)",
                "      : callbacks_(callbacks), user_data_(user_data), destroy_(destroy) {}",
                f"  ~{adapter}() override {{ if (destroy_) destroy_(user_data_); }}", ""]
        for method in interface.methods:
            result = types.resolve(method.returns).type
            nullable = isinstance(result, NullableType)
            value_kind = types.resolve(result.value).type.kind if nullable else result.kind
            returns = (f"std::optional<{'long long' if value_kind == 'int64' else 'std::string'}>"
                       if nullable else "std::string" if value_kind == "string" else "void")
            params = []
            call_args = ["user_data_"]
            for parameter in method.parameters:
                ref = types.resolve(parameter.type).type
                optional = isinstance(ref, NullableType)
                kind = types.resolve(ref.value).type.kind if optional else ref.kind
                value_type = "const std::optional<std::string>&" if optional else \
                    "const std::string&" if kind == "string" else "long long" if kind == "int64" else "int"
                params.append(f"{value_type} {parameter.name}")
                call_args.append(f"{parameter.name} ? {parameter.name}->c_str() : nullptr" if optional else
                                 f"{parameter.name}.c_str()" if kind == "string" else parameter.name)
            out += [f"  {returns} {method.name}({', '.join(params)}) override {{"]
            if nullable and value_kind == "int64":
                out += ["    int64_t value = 0;", "    int present = 0;"]
                call_args += ["&value", "&present"]
            elif value_kind == "string":
                out.append("    char* value = nullptr;")
                call_args.append("&value")
            out += ["    char* error = nullptr;",
                    f"    const int status = callbacks_.{method.name}({', '.join([*call_args, '&error'])});",
                    "    std::unique_ptr<char, decltype(&std::free)> error_owner(error, &std::free);"]
            if value_kind == "string":
                out.append("    std::unique_ptr<char, decltype(&std::free)> value_owner(value, &std::free);")
            fallback = f"{snake(interface.name.removesuffix('Provider'))} {method.name} failed"
            out += [f'    if (status != 0) throw std::runtime_error(error ? error : "{fallback}");']
            if nullable and value_kind == "int64":
                out += [f'    if (present != 0 && present != 1) throw std::runtime_error("{interface.name}.{method.name}: invalid presence flag");',
                        "    return present ? std::optional<long long>(value) : std::nullopt;"]
            elif nullable:
                out.append("    return value ? std::optional<std::string>(value) : std::nullopt;")
            elif value_kind == "string":
                out += [f'    if (!value) throw std::runtime_error("{interface.name}.{method.name}: null string result");',
                        "    return std::string(value);"]
            out += ["  }", ""]
        out += [" private:", f"  {callbacks} callbacks_;", "  void* user_data_;",
                "  holder_destroy_fn destroy_;", "};", ""]
    out += ["} // namespace", ""]
    for op in types.host_operations:
        interface = types.contract.interface(op.host)
        callbacks = f"holder_{snake(interface.name)}_callbacks"
        adapter = f"C{interface.name}Callbacks"
        missing = " || ".join(f"callbacks->{method.name} == nullptr" for method in interface.methods)
        out += [f"extern \"C\" int holder_{op.name}(const {callbacks}* callbacks, void* user_data,",
                "                                       holder_destroy_fn destroy_user_data, holder_error** out_error) {",
                "  if (out_error) *out_error = nullptr;",
                f"  std::shared_ptr<{adapter}> provider;", "  try {",
                f"    provider = std::make_shared<{adapter}>(callbacks ? *callbacks : {callbacks}{{}},",
                "                                           user_data, destroy_user_data);",
                "  } catch (const std::bad_alloc&) {",
                "    if (destroy_user_data) destroy_user_data(user_data);",
                '    return holder::contract::detail::fail(out_error, HOLDER_ERROR_ALLOCATION, "allocation failed");',
                "  }",
                f"  if (!callbacks || {missing})",
                f'    return holder::contract::detail::fail(out_error, HOLDER_ERROR_INVALID_ARGUMENT, "{interface.name} callbacks are required");',
                "  try {", f"    holder::contract::{op.name}(provider);", "    return HOLDER_OK;",
                "  } catch (const std::bad_alloc&) {",
                '    return holder::contract::detail::fail(out_error, HOLDER_ERROR_ALLOCATION, "allocation failed");',
                "  } catch (const std::exception& error) {",
                "    return holder::contract::detail::fail(out_error, HOLDER_ERROR_RUNTIME, error.what());",
                "  } catch (...) {",
                '    return holder::contract::detail::fail(out_error, HOLDER_ERROR_RUNTIME, "unknown holder error");',
                "  }", "}", ""]
    return "\n".join(out)
