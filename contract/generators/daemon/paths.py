"""Route paths: their regexes, specificity and the rule that no two routes take the same request.

Ported from RIG's ir.py (https://github.com/zeth/rig, src/rig/ir.py at e32ea68).
"""

from __future__ import annotations

import re

from compiler.ir import HttpBindingView

PARAMETER = re.compile(r"\{([A-Za-z_][A-Za-z_0-9]*)\}\Z")
LITERAL = re.compile(r"[A-Za-z0-9._~-]+\Z")


def route_pattern(path: str) -> tuple[str, dict[str, int]]:
    """The regex for a route and the capture position of each path parameter."""
    if not isinstance(path, str) or not path.startswith("/") or "//" in path or (path != "/" and path.endswith("/")):
        raise ValueError(f"unsupported route path: {path!r}")
    if path == "/":
        return "^/$", {}
    regex, captures = ["^"], {}
    for segment in path.split("/")[1:]:
        regex.append("/")
        placeholder = PARAMETER.fullmatch(segment)
        if placeholder:
            name = placeholder[1]
            if name in captures:
                raise ValueError(f"duplicate route parameter: {name}")
            regex.append("([^/]+)")
            captures[name] = len(captures) + 1
        elif LITERAL.fullmatch(segment):
            regex.append(re.escape(segment))
        else:
            raise ValueError(f"unsupported route segment: {segment}")
    regex.append("$")
    return "".join(regex), captures


def route_segments(path: str) -> tuple[tuple[int, str], ...]:
    """The route's segments as (specificity, literal): 0 for a literal, 1 for a parameter."""
    route_pattern(path)
    return tuple((1, "") if PARAMETER.fullmatch(segment) else (0, segment)
                 for segment in path.split("/")[1:] if segment)


def route_priority(binding: HttpBindingView) -> tuple[int, ...]:
    return tuple(kind for kind, _ in route_segments(binding.path))


def _segments_overlap(left, right) -> bool:
    if len(left) != len(right):
        return False
    return not any(a_kind == b_kind == 0 and a_value != b_value
                   for (a_kind, a_value), (b_kind, b_value) in zip(left, right))


def _same_template(left, right) -> bool:
    return all((a_kind == b_kind == 0 and a_value == b_value) or (a_kind != 0 and b_kind != 0)
               for (a_kind, a_value), (b_kind, b_value) in zip(left, right))


def _more_specific(left, right) -> bool:
    kinds = [(a[0], b[0]) for a, b in zip(left, right)]
    return all(a <= b for a, b in kinds) and any(a < b for a, b in kinds)


def validate_routes(bindings: tuple[HttpBindingView, ...] | list[HttpBindingView]) -> None:
    """Every request must match at most one route, or one route that is strictly more specific."""
    routes: dict[tuple[str, str], tuple] = {}
    for binding in bindings:
        method, path = binding.method, binding.path
        segments = route_segments(path)
        for (existing_method, existing_path), existing in routes.items():
            if method != existing_method or not _segments_overlap(segments, existing):
                continue
            if _same_template(segments, existing) or not (
                    _more_specific(segments, existing) or _more_specific(existing, segments)):
                raise ValueError(f"ambiguous routes: {method} {existing_path} and {method} {path}")
        routes[(method, path)] = segments
