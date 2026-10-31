"""Android adapters for permanent deletion of trashed cards."""

from .android_ir import ContractView, LiteralType, NamedType
from .android_templates import TEMPLATES


def generate_trash(contract: ContractView) -> tuple[str, str]:
    purge = contract.operation("delete_project_trash_item")
    empty = contract.operation("empty_trash")
    if [p.name for p in purge.parameters] != ["project_id", "type", "id"] or \
            purge.returns != NamedType(name="TrashDeleteResult") or \
            purge.parameter("type").type != NamedType(name="TrashItemType"):
        raise ValueError("Android card purge needs delete_project_trash_item")
    if [p.name for p in empty.parameters] != ["project_id", "type"] or \
            empty.returns != NamedType(name="TrashEmptyResult") or \
            empty.parameter("type").type != NamedType(name="TrashFilter"):
        raise ValueError("Android empty card trash needs empty_trash")
    for alias in ("TrashItemType", "TrashFilter"):
        values = contract.alias(alias)
        if not isinstance(values, LiteralType) or "card" not in values.values:
            raise ValueError(f"Android card trash needs {alias} to include card")
    return (TEMPLATES.get_template("android_trash.kt.j2").render(),
            TEMPLATES.get_template("android_trash.cpp.j2").render())
