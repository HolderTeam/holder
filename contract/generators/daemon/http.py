"""Plan HTTP dispatch from the same typed operations used by other targets."""

from __future__ import annotations

from dataclasses import dataclass

from compiler.ir import ContractView, HttpBindingView, OperationView, ParameterView


@dataclass(frozen=True)
class HttpArgument:
    parameter: ParameterView
    location: str


@dataclass(frozen=True)
class HttpRoute:
    operation: OperationView
    binding: HttpBindingView
    arguments: tuple[HttpArgument, ...]


@dataclass(frozen=True)
class HttpPlan:
    routes: tuple[HttpRoute, ...]
    bindings: tuple[HttpBindingView, ...]
    error_statuses: dict[str, int]


def plan_http(contract: ContractView) -> HttpPlan:
    """Match declared bindings to operations and locate every argument."""
    operations = {operation.name: operation for operation in contract.operations}
    routes = []
    for binding in contract.http:
        operation = operations.get(binding.operation)
        if operation is None or operation.daemon_only:
            # The HTTP contract can also describe routes whose implementations are
            # outside the generated core dispatcher.
            continue
        path = set(binding.path_parameters)
        query = set(binding.query)
        if path & query:
            raise ValueError(f"{operation.name}: parameter appears in path and query: {sorted(path & query)}")
        declared = {parameter.name for parameter in operation.parameters}
        unknown = (path | query) - declared
        if unknown:
            raise ValueError(f"{operation.name}: HTTP binding names unknown parameters: {sorted(unknown)}")
        arguments = tuple(HttpArgument(parameter, "path" if parameter.name in path else
                                       "query" if parameter.name in query else "body")
                          for parameter in operation.parameters)
        routes.append(HttpRoute(operation, binding, arguments))
    return HttpPlan(tuple(routes), tuple(contract.http),
                    {error.error: error.status for error in contract.http_errors})
