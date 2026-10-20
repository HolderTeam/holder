RIG ?= rig
PYTHON ?= python3
ROOT := $(abspath ../..)

.DELETE_ON_ERROR:
.PHONY: check FORCE

build/contract-ir.json: FORCE
	mkdir -p build
	$(RIG) inspect --header daemon/src/api/RigContract.h --namespace holder::api -- -I$(ROOT) -I$(ROOT)/daemon/src > $@

build/contract-openapi.json: build/contract-ir.json
	$(RIG) openapi --omit-operation-ids < $< > $@

check: build/contract-openapi.json
	$(PYTHON) check_contract.py $(ROOT)/api/openapi.yaml build/contract-openapi.json
