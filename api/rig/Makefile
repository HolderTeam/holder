RIG ?= rig
PYTHON ?= python3
ROOT := $(abspath ../..)

.DELETE_ON_ERROR:
.PHONY: check FORCE

build/bye-ir.json: FORCE
	mkdir -p build
	$(RIG) inspect --header daemon/src/api/Bye.h --namespace holder::api -- -I$(ROOT) -I$(ROOT)/daemon/src > $@

build/bye-openapi.json: build/bye-ir.json
	$(RIG) openapi --title 'Holder API' --omit-operation-ids < $< > $@

check: build/bye-openapi.json
	$(PYTHON) check_bye.py $(ROOT)/api/openapi.yaml build/bye-openapi.json
