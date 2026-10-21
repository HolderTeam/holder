RIG ?= rig
PYTHON ?= python3
ROOT := $(abspath ../..)

.DELETE_ON_ERROR:
.PHONY: check check-generated update-generated FORCE

build/contract-ir.json: FORCE
	mkdir -p build
	$(RIG) inspect --header daemon/src/api/RigContract.h --namespace holder::api --namespace holder::api::routes -- -I$(ROOT) -I$(ROOT)/daemon/src > $@

build/contract-openapi.json: build/contract-ir.json
	$(RIG) openapi --omit-operation-ids < $< > $@

check: build/contract-openapi.json
	$(PYTHON) check_contract.py $(ROOT)/api/openapi.yaml build/contract-openapi.json

build/bye-ir.json: FORCE
	mkdir -p build
	$(RIG) inspect --header daemon/src/api/Bye.h --namespace holder::api -- -I$(ROOT) -I$(ROOT)/daemon/src > $@

build/bye-json.hpp: build/bye-ir.json
	$(RIG) json-codec --api-header api/Bye.h < $< > $@

build/bye-routes.hpp: build/bye-ir.json build/bye-json.hpp
	$(RIG) http-routes --codec-header api/generated/ByeJson.h < $< > $@

build/project-tags-ir.json: FORCE
	mkdir -p build
	$(RIG) inspect --header daemon/src/api/ProjectTags.h --namespace holder::api -- -I$(ROOT) -I$(ROOT)/daemon/src > $@

build/project-tags-json.hpp: build/project-tags-ir.json
	$(RIG) json-codec --api-header api/ProjectTags.h < $< > $@

build/project-tags-routes.hpp: build/project-tags-ir.json build/project-tags-json.hpp
	$(RIG) http-routes --codec-header api/generated/ProjectTagsJson.h < $< > $@

check-generated: build/bye-routes.hpp build/project-tags-routes.hpp
	cmp build/bye-json.hpp $(ROOT)/daemon/src/api/generated/ByeJson.h
	cmp build/bye-routes.hpp $(ROOT)/daemon/src/api/generated/ByeRoutes.h
	cmp build/project-tags-json.hpp $(ROOT)/daemon/src/api/generated/ProjectTagsJson.h
	cmp build/project-tags-routes.hpp $(ROOT)/daemon/src/api/generated/ProjectTagsRoutes.h

update-generated: build/bye-routes.hpp build/project-tags-routes.hpp
	mkdir -p $(ROOT)/daemon/src/api/generated
	cp build/bye-json.hpp $(ROOT)/daemon/src/api/generated/ByeJson.h
	cp build/bye-routes.hpp $(ROOT)/daemon/src/api/generated/ByeRoutes.h
	cp build/project-tags-json.hpp $(ROOT)/daemon/src/api/generated/ProjectTagsJson.h
	cp build/project-tags-routes.hpp $(ROOT)/daemon/src/api/generated/ProjectTagsRoutes.h
