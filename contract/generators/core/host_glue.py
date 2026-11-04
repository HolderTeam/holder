"""C callback tables and ownership adapters for contract host interfaces."""

from __future__ import annotations

from compiler.ir import LiteralType, NullableType

from ..common import contract_sources, header_lines
from .abi import literal_member, snake
from .render import c_signature
from .types import CoreTypes, camel


def generate_host_glue(types: CoreTypes) -> str:
    out = [*header_lines(contract_sources(types.contract),
                         "The C callback adapters for host-implemented contract interfaces.", "cpp"),
           '#include "Contract.h"', '#include "holder/holder_api.h"', "", "#include <cstdint>",
           "#include <cstdlib>", "#include <memory>", "#include <new>", "#include <optional>",
           "#include <stdexcept>", "#include <string>", "#include <vector>", "", "namespace {", ""]
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
                       if nullable else "std::vector<unsigned char>" if value_kind == "bytes" else
                       "std::string" if value_kind == "string" else "bool" if value_kind == "bool" else "void")
            params = []
            call_args = ["user_data_"]
            for parameter in method.parameters:
                ref = types.resolve(parameter.type).type
                optional = isinstance(ref, NullableType)
                kind = types.resolve(ref.value).type.kind if optional else ref.kind
                value_type = "const std::optional<std::string>&" if optional else \
                    "const std::vector<unsigned char>&" if kind == "bytes" else \
                    "const std::string&" if kind == "string" else "long long" if kind == "int64" else "int"
                params.append(f"{value_type} {parameter.name}")
                if kind == "bytes":
                    call_args.extend([f"{parameter.name}.data()", f"{parameter.name}.size()"])
                else:
                    call_args.append(f"{parameter.name} ? {parameter.name}->c_str() : nullptr" if optional else
                                     f"{parameter.name}.c_str()" if kind == "string" else parameter.name)
            out += [f"  {returns} {method.name}({', '.join(params)}) override {{"]
            if nullable and value_kind == "int64":
                out += ["    int64_t value = 0;", "    int present = 0;"]
                call_args += ["&value", "&present"]
            elif value_kind == "string":
                out.append("    char* value = nullptr;")
                call_args.append("&value")
            elif value_kind == "bytes":
                out += ["    unsigned char* value = nullptr;", "    size_t value_len = 0;"]
                call_args += ["&value", "&value_len"]
            elif value_kind == "bool":
                out.append("    int value = 0;")
                call_args.append("&value")
            if interface.error_codes:
                out.append("    int error_code = 0;")
                call_args.append("&error_code")
            out += ["    char* error = nullptr;",
                    f"    const int status = callbacks_.{method.name}({', '.join([*call_args, '&error'])});",
                    "    std::unique_ptr<char, decltype(&std::free)> error_owner(error, &std::free);"]
            if value_kind == "string":
                out.append("    std::unique_ptr<char, decltype(&std::free)> value_owner(value, &std::free);")
            elif value_kind == "bytes":
                out.append("    std::unique_ptr<unsigned char, decltype(&std::free)> value_owner(value, &std::free);")
            fallback = f"{snake(interface.name.removesuffix('Provider'))} {method.name} failed"
            if interface.error_codes:
                out += [f'    if (status != 0) throw holder::contract::{interface.name}Failure(',
                        f'        error_code >= 0 && error_code < {len(interface.error_codes)} ? error_code : 0,',
                        f'        error ? error : "{fallback}");']
            else:
                out += [f'    if (status != 0) throw std::runtime_error(error ? error : "{fallback}");']
            if nullable and value_kind == "int64":
                out += [f'    if (present != 0 && present != 1) throw std::runtime_error("{interface.name}.{method.name}: invalid presence flag");',
                        "    return present ? std::optional<long long>(value) : std::nullopt;"]
            elif nullable:
                out.append("    return value ? std::optional<std::string>(value) : std::nullopt;")
            elif value_kind == "string":
                out += [f'    if (!value) throw std::runtime_error("{interface.name}.{method.name}: null string result");',
                        "    return std::string(value);"]
            elif value_kind == "bytes":
                out += [f'    if (!value && value_len != 0) throw std::runtime_error("{interface.name}.{method.name}: null byte result");',
                        "    return value_len ? std::vector<unsigned char>(value, value + value_len) : std::vector<unsigned char>{};"]
            elif value_kind == "bool":
                out += [f'    if (value != 0 && value != 1) throw std::runtime_error("{interface.name}.{method.name}: invalid boolean result");',
                        "    return value == 1;"]
            out += ["  }", ""]
        out += [" private:", f"  {callbacks} callbacks_;", "  void* user_data_;",
                "  holder_destroy_fn destroy_;", "};", ""]
    out += ["} // namespace", ""]
    for op in types.host_operations:
        interface = types.contract.interface(op.host)
        callbacks = f"holder_{snake(interface.name)}_callbacks"
        adapter = f"C{interface.name}Callbacks"
        missing = " || ".join(f"callbacks->{method.name} == nullptr" for method in interface.methods)
        out += [f'extern "C" {c_signature(types, op)} {{',
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
                ]
        if op.context == "required":
            out += ["  if (!context)",
                    '    return holder::contract::detail::fail(out_error, HOLDER_ERROR_INVALID_ARGUMENT, "context is required");']
        call_args = ["context"] if op.context == "required" else []
        for parameter in op.parameters:
            resolved = types.resolve(parameter.type).type
            if isinstance(resolved, LiteralType):
                # A choice of literals is a C enum; zero and unlisted values are rejected.
                prefix, type_name = f"holder_{op.name}_{parameter.name}".upper(), camel(op.name) + camel(parameter.name)
                out += [f"  holder::contract::{type_name} {parameter.name}_value;", f"  switch ({parameter.name}) {{"]
                out += [f"  case {prefix}_{literal_member(value)}: {parameter.name}_value = "
                        f"holder::contract::{type_name}::{camel(str(value))}; break;" for value in resolved.values]
                out += ["  default:",
                        f'    return holder::contract::detail::fail(out_error, HOLDER_ERROR_INVALID_ARGUMENT, "{parameter.name} is not a {prefix.lower()} value");',
                        "  }"]
                call_args.append(f"{parameter.name}_value")
                continue
            kind = resolved.kind
            out += [f"  if (!{parameter.name}" + (f" || {parameter.name}[0] == '\\0'" if kind == "string"
                                             else f" || {parameter.name}_len == 0") + ")",
                    f'    return holder::contract::detail::fail(out_error, HOLDER_ERROR_INVALID_ARGUMENT, "{parameter.name} is required");']
            call_args.append(f"std::string({parameter.name})" if kind == "string" else
                             f"std::vector<unsigned char>({parameter.name}, {parameter.name} + {parameter.name}_len)")
        call_args.append("provider")
        out += ["  try {", f"    holder::contract::{op.name}({', '.join(call_args)});", "    return HOLDER_OK;",
                "  } catch (const std::bad_alloc&) {",
                '    return holder::contract::detail::fail(out_error, HOLDER_ERROR_ALLOCATION, "allocation failed");',
                "  } catch (const std::invalid_argument& error) {",
                "    return holder::contract::detail::fail(out_error, HOLDER_ERROR_INVALID_ARGUMENT, error.what());",
                "  } catch (const std::exception& error) {",
                "    return holder::contract::detail::fail(out_error, HOLDER_ERROR_RUNTIME, error.what());",
                "  } catch (...) {",
                '    return holder::contract::detail::fail(out_error, HOLDER_ERROR_RUNTIME, "unknown holder error");',
                "  }", "}", ""]
    return "\n".join(out)
