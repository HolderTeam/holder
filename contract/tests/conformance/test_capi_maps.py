"""The conformance transport must match the generated C layouts."""

from pathlib import Path

import cffi

from compiler.ir import ContractView, NamedType
from generators.core.types import CoreTypes
from holder_capi import Core, declarations
from test_support.contract import extract, generate


def transport():
    ir = extract(Path(__file__).resolve().parents[2], "interface")
    core = Core.__new__(Core)
    core.model = CoreTypes(ContractView.model_validate(ir))
    core.ffi = cffi.FFI()
    core.ffi.cdef(declarations(generate(ir)["holder_api.h"]))
    return core


def test_c_api_transport_encodes_string_and_string_list_maps():
    core = transport()

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


def test_c_api_transport_decodes_optional_list_with_nullable_record():
    core = transport()
    sync_type = core.model.instance(NamedType(name="SyncState"), {})
    state = core.ffi.new("holder_sync_state*")
    assert "conflict_copies" not in core.from_c(sync_type, state[0])

    original_id = core.ffi.new("char[]", b"original")
    copy_id = core.ffi.new("char[]", b"copy")
    title = core.ffi.new("char[]", b"Renamed copy")
    items = core.ffi.new("holder_conflict_copy[]", [{
        "original_card_id": original_id,
        "copy_card": {"card_id": copy_id, "title": title},
    }])
    copies = core.ffi.new("holder_conflict_copy_list*", {"items": items, "count": 1})
    state.conflict_copies = copies

    expected = {
        "original_card_id": "original",
        "original": None,
        "copy_card": {"card_id": "copy", "title": "Renamed copy", "deleted_at": None},
    }
    assert core.from_c(sync_type, state[0])["conflict_copies"] == [expected]

    original = core.ffi.new("holder_card_summary*", {"card_id": original_id, "title": title})
    items[0].original = original
    expected["original"] = {"card_id": "original", "title": "Renamed copy", "deleted_at": None}
    assert core.from_c(sync_type, state[0])["conflict_copies"] == [expected]
