# The contract's tooling. Needs Python 3.12 or later with requirements.txt installed.
PYTHON ?= python3

.PHONY: check lint test ir update

# Everything CI runs: lint, the tooling's tests, and a check of the contract itself, which fails
# when core's generated files are out of date.
check: lint test
	$(PYTHON) -m compiler check

lint:
	$(PYTHON) -m ruff check .

test:
	$(PYTHON) -m pytest

ir:
	$(PYTHON) -m compiler ir --output build/ir.json

# After changing the contract or the generator: writes core's C API files, for review and commit.
update:
	$(PYTHON) -m compiler update
