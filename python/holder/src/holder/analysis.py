"""Optional pandas and NetworkX adapters for detached project records."""

from __future__ import annotations

from collections.abc import Iterable, Mapping, Sequence
from copy import deepcopy
from importlib import import_module
from types import ModuleType
from typing import TYPE_CHECKING, TypedDict

from ._workspace_contract import CardListItem, Link, Milestone, Project
from .exports import ProjectRecords, TagMembership

if TYPE_CHECKING:
    import networkx as nx
    import pandas as pd


class ProjectDataFrames(TypedDict):
    """Project records as five pandas tables."""

    projects: pd.DataFrame
    cards: pd.DataFrame
    links: pd.DataFrame
    tag_memberships: pd.DataFrame
    milestones: pd.DataFrame


def _require_extra(package: str, extra: str) -> ModuleType:
    try:
        return import_module(package)
    except ModuleNotFoundError as error:
        if error.name != package:
            raise
        raise ModuleNotFoundError(
            f'{package} is required for this adapter. Install "holder-python[{extra}]".',
            name=package,
        ) from None


def _frame(
    pandas: ModuleType,
    rows: Iterable[Mapping[str, object]],
    columns: Sequence[str],
) -> pd.DataFrame:
    frame = pandas.DataFrame.from_records(rows, columns=columns)
    for column in columns:
        if column in {"created_at", "updated_at", "deleted_at"}:
            frame[column] = pandas.to_datetime(
                frame[column], unit="s", utc=True
            ).astype("datetime64[ns, UTC]")
        elif column == "sort_key":
            frame[column] = frame[column].astype("float64")
        elif column == "child_count":
            frame[column] = frame[column].astype("Int64")
        elif column == "dates":
            frame[column] = frame[column].astype("object")
        else:
            frame[column] = frame[column].astype("string")
    return frame


def to_dataframes(
    records: ProjectRecords | Iterable[ProjectRecords],
) -> ProjectDataFrames:
    """Convert one or more project exports to detached pandas tables with UTC timestamps.

    The cards table includes a content column when the records contain bodies.
    Milestone dates keep their role and original day or instant.
    """
    pandas = _require_extra("pandas", "pandas")
    projects = [records] if isinstance(records, dict) else list(records)
    cards = [card for project in projects for card in project["cards"]]
    have_content = any("content" in card for card in cards)
    card_columns = [
        field
        for field in CardListItem.__annotations__
        if field != "content" or have_content
    ]
    milestones = (
        {**row, "dates": deepcopy(row["dates"])}
        for project in projects
        for row in project["milestones"]
    )
    return {
        "projects": _frame(
            pandas,
            (project["project"] for project in projects),
            list(Project.__annotations__),
        ),
        "cards": _frame(pandas, cards, card_columns),
        "links": _frame(
            pandas,
            (link for project in projects for link in project["links"]),
            list(Link.__annotations__),
        ),
        "tag_memberships": _frame(
            pandas,
            (tag for project in projects for tag in project["tag_memberships"]),
            list(TagMembership.__annotations__),
        ),
        "milestones": _frame(pandas, milestones, list(Milestone.__annotations__)),
    }


def to_networkx(records: ProjectRecords | Iterable[ProjectRecords]) -> nx.MultiDiGraph:
    """Build a directed multigraph of live cards and explicit card-to-card links.

    A node is keyed by (project_id, card_id). Targets outside the export get a
    placeholder node with exported=False. Link kind is the multigraph edge key.
    """
    networkx = _require_extra("networkx", "graph")
    graph = networkx.MultiDiGraph()
    projects = (records,) if isinstance(records, dict) else records
    for project in projects:
        for card in project["cards"]:
            node = (card["project_id"], card["card_id"])
            graph.add_node(node, **card, exported=True)
        for link in project["links"]:
            if link["source_type"] != "card" or link["target_type"] != "card":
                continue
            source = (link["source_project_id"], link["source_id"])
            target = (link["target_project_id"], link["target_id"])
            if target not in graph:
                graph.add_node(
                    target, project_id=target[0], card_id=target[1], exported=False
                )
            graph.add_edge(source, target, key=link["kind"], **link)
    return graph
