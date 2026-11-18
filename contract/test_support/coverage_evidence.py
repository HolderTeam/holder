"""Optional pytest observations included in conformance JUnit reports."""

from contextvars import ContextVar

observations: ContextVar[set[str] | None] = ContextVar("coverage_observations", default=None)


def observe(kind: str, operation: str) -> None:
    seen = observations.get()
    if seen is not None:
        seen.add(f"{kind}:{operation}")
