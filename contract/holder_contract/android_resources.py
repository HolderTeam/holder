"""Generate Android's resource listing bridge over the contract C ABI."""

from .android_models import map_record
from .android_templates import TEMPLATES
from .ir import ContractView, ListType, NamedType

KOTLIN_PATH = "apps/mobile/android/app/src/main/java/team/holder/android/generated/ContractResources.kt"
JNI_PATH = "apps/mobile/android/app/src/main/cpp/generated/contract_resources.cpp"


def generate_resources(contract: ContractView) -> dict[str, str]:
    # Android only uses resource display fields and placement IDs for retrieval. The
    # projection intentionally omits metadata and legacy storage-provider internals.
    operation = contract.operation("list_resources")
    if [p.name for p in operation.parameters] != ["project_id"] or \
            operation.returns != ListType(kind="list", items=NamedType(name="Resource")):
        raise ValueError("Android resource listing needs a generator update")

    expected = {
        "AssetPlacement": ["placement_id", "location_id", "encoding", "stored_byte_size", "created_at"],
        "ResourceAsset": ["asset_id", "original_filename", "media_type", "byte_size",
                          "plaintext_sha256", "placements", "created_at", "updated_at"],
        "Resource": ["resource_id", "project_id", "type", "label", "metadata", "assets",
                     "created_at", "updated_at"],
    }
    for name, fields in expected.items():
        if [field.name for field in contract.record(name).fields] != fields:
            raise ValueError(f"Android {name} fields need a generator update")

    placement = map_record(contract.record("AssetPlacement"), contract, allow_literal_fields={"encoding"})
    asset = map_record(contract.record("ResourceAsset"), contract)
    resource = map_record(contract.record("Resource"), contract, exclude_fields={"metadata"})
    records = f"{placement.render()}\n{asset.render()}\n{resource.render()}\n"

    header = "// Generated from contract/interface/resources.py and contract/interface/app.py by holder_contract.\n" \
             "// Do not edit; run make -C contract update.\n"
    return {
        KOTLIN_PATH: header + TEMPLATES.get_template("android_resources.kt.j2").render(records=records),
        JNI_PATH: header + TEMPLATES.get_template("android_resources.cpp.j2").render(),
    }
