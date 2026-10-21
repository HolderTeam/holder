RIG ?= rig
PYTHON ?= python3
ROOT := $(abspath ../..)
# The daemon compiles against core's installed headers; inspection needs them too.
CORE_INCLUDE ?= $(ROOT)/core/out/install/local/include

.DELETE_ON_ERROR:
.PHONY: check check-generated update-generated FORCE

build/contract-ir.json: FORCE
	mkdir -p build
	$(RIG) inspect --header daemon/src/api/RigContract.h --namespace holder::api --namespace holder::api::routes -- -I$(ROOT) -I$(ROOT)/daemon/src -I$(CORE_INCLUDE) -I$(CORE_INCLUDE)/holder > $@

build/contract-openapi.json: build/contract-ir.json
	$(RIG) openapi --omit-operation-ids < $< > $@

check: build/contract-openapi.json
	$(PYTHON) check_contract.py $(ROOT)/api/openapi.yaml build/contract-openapi.json

build/runtime-ir.json: FORCE
	mkdir -p build
	$(RIG) inspect --header daemon/src/api/RigRuntime.h --namespace holder::api --namespace holder::api::routes -- -I$(ROOT) -I$(ROOT)/daemon/src -I$(CORE_INCLUDE) -I$(CORE_INCLUDE)/holder > $@

build/runtime-json.hpp: build/runtime-ir.json
	$(RIG) json-codec --api-header api/RigRuntime.h < $< > $@

build/runtime-routes.hpp: build/runtime-ir.json build/runtime-json.hpp
	$(RIG) http-routes --codec-header api/generated/RuntimeJson.h < $< > $@

check-generated: build/runtime-routes.hpp
	cmp build/runtime-json.hpp $(ROOT)/daemon/src/api/generated/RuntimeJson.h
	cmp build/runtime-routes.hpp $(ROOT)/daemon/src/api/generated/RuntimeRoutes.h

update-generated: build/runtime-routes.hpp
	mkdir -p $(ROOT)/daemon/src/api/generated
	cp build/runtime-json.hpp $(ROOT)/daemon/src/api/generated/RuntimeJson.h
	cp build/runtime-routes.hpp $(ROOT)/daemon/src/api/generated/RuntimeRoutes.h
