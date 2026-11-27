import sys

import pytest

from holder import ProjectRecords, to_dataframes, to_networkx


@pytest.fixture
def records() -> ProjectRecords:
    return {
        "project": {
            "project_id": "one",
            "name": "Research",
            "root_path": "/research",
            "git_remote_url": None,
            "git_provider": None,
            "privacy_mode": "plain",
            "project_key_id": None,
            "created_at": 1,
            "updated_at": 2,
        },
        "cards": [],
        "links": [],
        "tag_memberships": [],
        "milestones": [],
    }


@pytest.mark.parametrize(
    ("adapter", "package", "extra"),
    [(to_dataframes, "pandas", "pandas"), (to_networkx, "networkx", "graph")],
)
def test_adapters_explain_missing_extra(records, monkeypatch, adapter, package, extra):
    monkeypatch.setitem(sys.modules, package, None)
    with pytest.raises(ModuleNotFoundError, match=rf"holder-python\[{extra}\]"):
        adapter(records)


def test_dataframes_keep_typed_columns_and_detach_dates(records):
    pandas = pytest.importorskip("pandas")
    empty = to_dataframes(records)
    assert all(
        empty[key].empty for key in ("cards", "links", "tag_memberships", "milestones")
    )
    assert "content" not in empty["cards"]
    assert str(empty["cards"]["child_count"].dtype) == "Int64"
    assert str(empty["links"]["created_at"].dtype) == "datetime64[ns, UTC]"
    assert empty["projects"].loc[0, "updated_at"] == pandas.Timestamp(
        2, unit="s", tz="UTC"
    )
    no_projects = to_dataframes(iter(()))
    assert all(table.empty for table in no_projects.values())
    assert no_projects["projects"].dtypes.equals(empty["projects"].dtypes)

    records["milestones"].append(
        {
            "project_id": "one",
            "card_id": "card",
            "milestone_id": "milestone",
            "dates": [{"role": "due", "date": "2026-11-30"}],
            "kind": None,
            "description": "Read paper",
            "created_at": 1,
            "updated_at": 2,
        }
    )
    dates = to_dataframes(records)["milestones"].loc[0, "dates"]
    assert dates == records["milestones"][0]["dates"]
    dates[0]["role"] = "changed"
    assert records["milestones"][0]["dates"][0]["role"] == "due"


def test_networkx_uses_project_scoped_ids_and_keeps_parallel_links(records):
    pytest.importorskip("networkx")
    assert to_networkx(records).number_of_nodes() == 0

    for card_id in ("shared", "isolated"):
        records["cards"].append(
            {
                "card_id": card_id,
                "project_id": "one",
                "title": card_id,
                "rel_path": f"cards/{card_id}.md",
                "parent_card_id": None,
                "sort_key": 1.0,
                "created_at": 1,
                "updated_at": 2,
                "deleted_at": None,
            }
        )
    for kind in ("supports", "ref"):
        records["links"].append(
            {
                "source_project_id": "one",
                "source_type": "card",
                "source_id": "shared",
                "target_project_id": "two",
                "target_type": "card",
                "target_id": "shared",
                "kind": kind,
                "label": "Evidence",
                "created_at": 1,
            }
        )
    records["links"].append({**records["links"][0], "target_type": "resource"})

    graph = to_networkx(records)
    source = ("one", "shared")
    target = ("two", "shared")
    assert set(graph) == {source, target, ("one", "isolated")}
    assert graph.nodes[source]["exported"] is True
    assert graph.nodes[target] == {
        "project_id": "two",
        "card_id": "shared",
        "exported": False,
    }
    assert set(graph[source][target]) == {"supports", "ref"}
    assert graph[source][target]["ref"]["label"] == "Evidence"
    graph.nodes[source]["title"] = "Edited"
    assert records["cards"][0]["title"] == "shared"

    other = {
        **records,
        "project": {**records["project"], "project_id": "two"},
        "cards": [
            {**records["cards"][0], "project_id": "two", "title": "External card"}
        ],
        "links": [],
    }
    for selection in ((records, other), (other, records)):
        combined = to_networkx(iter(selection))
        assert combined.nodes[target]["exported"] is True
        assert combined.nodes[target]["title"] == "External card"
        assert set(combined[source][target]) == {"supports", "ref"}
