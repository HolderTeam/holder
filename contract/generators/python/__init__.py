"""Generate shared Python workspace types from the compiler result."""

import json
from textwrap import wrap

from compiler.ir import ContractView, TypeRef
from generators.common import Unsupported
from generators.core.abi import snake

PATH = "python/holder/src/holder/_workspace_contract.py"
OPERATIONS = ("list_cards", "search_cards", "get_card", "get_card_metadata", "get_card_breadcrumbs", "resolve_card_reference", "create_card", "update_card", "move_card", "trash_card", "restore_card", "add_card_tag", "remove_card_tag", "list_card_links", "list_card_backlinks", "add_card_link", "remove_card_link", "list_card_milestones", "add_card_milestone", "update_card_milestone", "remove_card_milestone")

SCALARS = {"string": "str", "int32": "int", "int64": "int", "float64": "float", "bool": "bool"}


def annotation(ref: TypeRef) -> str:
    match ref.kind:
        case "named":
            if ref.arguments:
                return f"{ref.name}[{', '.join(map(annotation, ref.arguments))}]"
            return ref.name
        case "parameter":
            return ref.name
        case "nullable":
            return f"{annotation(ref.value)} | None"
        case "list":
            return f"list[{annotation(ref.items)}]"
        case "map":
            return f"dict[str, {annotation(ref.values)}]"
        case "literal":
            return f"Literal[{', '.join(map(json.dumps, ref.values))}]"
        case "union":
            return " | ".join(map(annotation, ref.alternatives))
        case kind if kind in SCALARS:
            return SCALARS[kind]
        case _:
            raise Unsupported(f"Python Card type: {ref.kind}")


def docstring(text: str, indent: str) -> list[str]:
    if "\n" not in text and len(text) + len(indent) < 88:
        return [f'{indent}"""{text}"""']
    paragraphs = text.split("\n\n")
    lines = [f'{indent}"""']
    for paragraph in paragraphs:
        if len(lines) > 1:
            lines.append("")
        lines.extend(wrap(paragraph, width=88, initial_indent=indent, subsequent_indent=indent))
    return [*lines, f'{indent}"""']


def generate_python_workspace(ir: ContractView) -> str:
    operations = [ir.operation(name) for name in OPERATIONS]
    lines = [
        '"""Generated from the Holder contract. Run make -C contract update."""',
        "",
        "from typing import Generic, Literal, NotRequired, Protocol, TypedDict, TypeVar",
        "",
    ]
    for name in ("ProjectId", "ProjectName", "GitRemoteUrl", "PrivacyMode", "CardId", "Tag", "TrashItemType", "LinkKind", "LinkTargetType", "LinkTargetId", "MilestoneId", "MilestoneRole", "Day", "DateTime", "MilestoneText", "ResourceId", "ResourceType", "ResourceMetadata"):
        lines.append(f"{name} = {annotation(ir.alias(name))}")
    lines.extend(["", 'T = TypeVar("T")'])
    for name in ("CardMetadata", "CardBreadcrumb", "TagOccurrence", "CardListItem", "Card", "NewCard", "CardUpdate", "CardMove", "Page", "Project", "TrashItem", "TrashDeleteResult", "TagCount", "CardReferenceResolveRequest", "CardReferenceCandidate", "CardReferenceResolveData", "SearchCard", "Link", "NewLink", "LinkKindLabels", "MilestoneDate", "Milestone", "NewMilestone", "MilestoneUpdate", "CalendarMilestone", "Calendar", "AssetPlacement", "ResourceAsset", "Resource"):
        record = ir.record(name)
        bases = [record.base or "TypedDict"]
        if record.type_parameters:
            bases.append(f"Generic[{', '.join(record.type_parameters)}]")
        lines.extend(["", "", f"class {name}({', '.join(bases)}):", *docstring(record.doc or record.name, "    "), ""])
        for field in record.fields:
            hint = annotation(field.type)
            if field.omittable or "default" in field.model_fields_set:
                hint = f"NotRequired[{hint}]"
            lines.append(f"    {field.name}: {hint}")

    lines.extend(
        [
            "",
            "",
            "class CardOperations(Protocol):",
            '    """The Card operations shared by daemon and embedded sessions."""',
        ]
    )
    for operation in operations:
        lines.extend(["", "    def " + operation.name + "(", "        self,"])
        optional = False
        for parameter in operation.parameters:
            has_default = "default" in parameter.model_fields_set
            if has_default and not optional:
                lines.append("        *,")
                optional = True
            default = ""
            if has_default:
                value = parameter.default
                default = f" = {json.dumps(value) if isinstance(value, str) else repr(value)}"
            lines.append(f"        {parameter.name}: {annotation(parameter.type)}{default},")
        lines.extend(
            [
                f"    ) -> {annotation(operation.returns)}:",
                *docstring("\n\n".join(filter(None, (operation.summary, operation.description))), "        "),
                "        ...",
            ]
        )
    errors = {error for operation in operations for error in operation.errors}
    errors.update(ir.operation("delete_project_trash_item").errors)
    statuses = {item.error: item.status for item in ir.http_errors}
    lines.extend(["", "", "NATIVE_ERRORS = {"])
    for error in ir.errors:
        if error.name in errors:
            lines.append(f'    {error.code}: ({statuses[error.name]}, "{snake(error.name)}"),')
    lines.extend(["}", ""])
    return "\n".join(lines)
