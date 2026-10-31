"""Generate Android's card-history list bridge over the contract C ABI."""

from .android_ir import ContractView, NamedType
from .android_templates import TEMPLATES

KOTLIN_PATH = "apps/mobile/android/app/src/main/java/team/holder/android/generated/ContractHistory.kt"
JNI_PATH = "apps/mobile/android/app/src/main/cpp/generated/contract_history.cpp"


def generate_history(contract: ContractView) -> dict[str, str]:
    operation = contract.operation("list_card_history")
    if [field.name for field in operation.parameters] != ["project_id", "card_id", "limit", "cursor"] or \
            operation.returns != NamedType(name="CardHistoryPage"):
        raise ValueError("Android card-history list operation needs a generator update")
    expected = {
        "CardHistorySave": ["oid", "parent_oids", "authored_at", "committed_at", "message"],
        "HistoryAuthor": ["name", "email"],
        "CardHistoryEntry": ["first_oid", "last_oid", "parent_oids", "visible_parent_oids",
                             "author", "started_at", "ended_at", "kind", "summary", "commit_count",
                             "is_merge", "saves"],
        "CardHistoryPage": ["head_oid", "entries", "next_cursor", "scan_limited"],
    }
    for name, fields in expected.items():
        if [field.name for field in contract.record(name).fields] != fields:
            raise ValueError(f"Android {name} fields need a generator update")
    comparison = contract.operation("compare_card_history")
    if [field.name for field in comparison.parameters] != [
        "project_id", "card_id", "to_oid", "from_oid", "mode"
    ] or comparison.returns != NamedType(name="CardHistoryComparison"):
        raise ValueError("Android card-history comparison needs a generator update")
    comparison_fields = {
        "CardHistoryVersion": ["exists", "oid", "title", "body"],
        "CardHistoryDiffLine": ["origin", "text", "old_line", "new_line"],
        "CardHistoryComparison": ["before", "after", "summary", "lines", "truncated"],
    }
    for name, fields in comparison_fields.items():
        if [field.name for field in contract.record(name).fields] != fields:
            raise ValueError(f"Android {name} fields need a generator update")
    restore = contract.operation("restore_card_history_snapshot")
    if [field.name for field in restore.parameters] != ["project_id", "card_id", "oid"] or \
            restore.returns != NamedType(name="CardHistoryRestoreResult"):
        raise ValueError("Android card-history restore needs a generator update")
    if [field.name for field in contract.record("CardHistoryRestoreResult").fields] != [
        "card_id", "restored_from_oid", "result_oid", "title", "updated_at", "deleted_at"
    ]:
        raise ValueError("Android CardHistoryRestoreResult fields need a generator update")
    header = "// Generated from contract/interface/history.py and contract/interface/app.py by holder_contract.\n" \
             "// Do not edit; run make -C contract update.\n"
    return {
        KOTLIN_PATH: header + TEMPLATES.get_template("android_history.kt.j2").render(),
        JNI_PATH: header + TEMPLATES.get_template("android_history.cpp.j2").render(),
    }
