RIG ?= rig
# rig inspect needs a C++26 compiler with reflection, such as GCC 16.
CXX ?= g++
ROOT := $(abspath ../..)
# The daemon compiles against core's installed headers; inspection needs them too.
CORE_INCLUDE ?= $(ROOT)/core/out/install/local/include
GENERATED := $(ROOT)/daemon/src/api/generated

.DELETE_ON_ERROR:
.PHONY: check update FORCE

# One inspection of the declarations gives the contract and the daemon's generated code.
build/ir.json: FORCE
	mkdir -p build
	$(RIG) inspect --cxx $(CXX) --header daemon/src/api/RigRuntime.h --namespace holder::api --namespace holder::api::routes -- -I$(ROOT) -I$(ROOT)/daemon/src -I$(CORE_INCLUDE) -I$(CORE_INCLUDE)/holder > $@

# The document for the routes declared here. make -C contract update merges the contract's routes
# into it to write api/openapi.yaml and api/openapi.json, the API's document.
build/openapi.json: build/ir.json
	$(RIG) openapi --format json --omit-operation-ids < $< > $@

build/runtime-json.hpp: build/ir.json
	$(RIG) json-codec --api-header api/RigRuntime.h < $< > $@

build/runtime-routes.hpp: build/ir.json build/runtime-json.hpp
	$(RIG) http-routes --codec-header api/generated/RuntimeJson.h < $< > $@

# Fails when a declaration changed without `make update`, or a generated file was edited.
check: build/openapi.json build/runtime-routes.hpp
	cmp build/openapi.json $(ROOT)/api/rig/openapi.rig.json
	cmp build/runtime-json.hpp $(GENERATED)/RuntimeJson.h
	cmp build/runtime-routes.hpp $(GENERATED)/RuntimeRoutes.h

# Then run make -C contract update, which writes api/openapi.yaml and api/openapi.json from this.
update: build/openapi.json build/runtime-routes.hpp
	cp build/openapi.json $(ROOT)/api/rig/openapi.rig.json
	mkdir -p $(GENERATED)
	cp build/runtime-json.hpp $(GENERATED)/RuntimeJson.h
	cp build/runtime-routes.hpp $(GENERATED)/RuntimeRoutes.h
