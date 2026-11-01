"""The conformance transport must match each generated C map's value type."""

from pathlib import Path

import cffi

from compiler.ir import ContractView, NamedType
from generators.core.types import CoreTypes
from holder_capi import HAND_WRITTEN, Core, declarations
from holder_contract.extract import extract
from holder_contract.generate import generate


def test_c_api_transport_encodes_string_and_string_list_maps():
    ir = extract(Path(__file__).resolve().parents[2], "interface")
    core = Core.__new__(Core)
    core.model = CoreTypes(ContractView.model_validate(ir))
    core.ffi = cffi.FFI()
    core.ffi.cdef(HAND_WRITTEN + declarations(generate(ir)["holder_api.h"]))

    def slot(record, field):
        instance = core.model.instance(NamedType(name=record), {})
        return next(item for item in core.model.slots(instance) if item.name == field)

    keep = []
    configuration = core.field(slot("NewStorageLocation", "configuration"), {"bucket": "notes"}, keep)
    assert core.ffi.string(configuration.items[0].values) == b"notes"
    location_type = core.model.instance(NamedType(name="NewStorageLocation"), {})
    location = core.ffi.new(f"{location_type.c_name}*")
    location.configuration = configuration
    assert core.from_c(location_type, location[0])["configuration"] == {"bucket": "notes"}

    metadata = core.field(slot("NewResource", "metadata"), {"author": ["Ada", "Lin"]}, keep)
    values = metadata.items[0].values
    assert [core.ffi.string(values.items[i]).decode() for i in range(values.count)] == ["Ada", "Lin"]
    resource_type = core.model.instance(NamedType(name="NewResource"), {})
    resource = core.ffi.new(f"{resource_type.c_name}*")
    resource.metadata = metadata
    assert core.from_c(resource_type, resource[0])["metadata"] == {"author": ["Ada", "Lin"]}
