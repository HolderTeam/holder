"""Attach actual contract checks to each conformance test's JUnit result."""

import pytest

from test_support.coverage_evidence import observations


@pytest.hookimpl(tryfirst=True)
def pytest_runtest_protocol(item):
    # Schemathesis stores the operation in item.name while its JUnit name is shared
    # across routes. Record the name even when the test will skip before fixture setup.
    item.user_properties.append(("contract-case", item.name))


@pytest.fixture(autouse=True)
def conformance_observations(request):
    seen: set[str] = set()
    token = observations.set(seen)
    try:
        yield
    finally:
        observations.reset(token)
        request.node.user_properties.extend(("contract-check", item) for item in sorted(seen))
