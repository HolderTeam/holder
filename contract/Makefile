# The contract's tooling. Needs Python 3.12 or later with requirements.txt installed.
PYTHON ?= python3

.PHONY: check lint test ir

# Everything CI runs: lint, the tooling's tests, and a check of the contract itself.
check: lint test
	$(PYTHON) -m holder_contract check

lint:
	$(PYTHON) -m ruff check .

test:
	$(PYTHON) -m pytest

ir:
	$(PYTHON) -m holder_contract ir --output build/ir.json
