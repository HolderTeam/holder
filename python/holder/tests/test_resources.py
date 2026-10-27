"""Friendly resource, attachment, location, import, and download workflows."""

import json
from pathlib import Path

import httpx
import pytest
from test_library import card_payload, holder_for, ok, project_payload

from holder import Asset, ImportJob, Resource, StorageLocation
from holder.exceptions import APIError, AuthenticationError, ProtocolError


def resource_data(project_id="p-0", *, assets=None):
    return {
        "resource_id": "r-1",
        "project_id": project_id,
        "type": "file",
        "label": "Report",
        "metadata": {"creator": ["Ada", "Grace"]},
        "assets": assets or [],
        "referenced_by_cards": [
            {
                "card_id": "c-0000",
                "title": "Card 0",
                "updated_at": 2,
                "link_kinds": ["attachment"],
            }
        ],
        "created_at": 1,
        "updated_at": 2,
    }


def asset_data():
    return {
        "asset_id": "a-1",
        "resource_id": "r-1",
        "original_filename": "report.pdf",
        "media_type": "application/pdf",
        "byte_size": 6,
        "plaintext_sha256": "a" * 64,
        "placements": [
            {
                "placement_id": "place-1",
                "location_id": "l-1",
                "encoding": "plain",
                "stored_byte_size": 6,
                "created_at": 1,
            }
        ],
        "created_at": 1,
        "updated_at": 2,
    }


def location_data(project_id="p-0", *, bound=False):
    return {
        "location_id": "l-1",
        "project_id": project_id,
        "name": "Local",
        "provider": "local_directory",
        "configuration": {},
        "bound": bound,
        "binding_preview": "/store" if bound else None,
        "created_at": 1,
        "updated_at": 2,
    }


def test_resources_and_card_attachments_are_scoped_and_paginated():
    requests = []
    state = resource_data()

    def respond(request):
        path = request.url.path
        if path == "/projects/p-0":
            return ok(project_payload())
        if path == "/projects/p-0/cards/c-0000":
            return ok(card_payload(content=True))
        requests.append(request)
        if path == "/resources" and request.method == "GET":
            assert request.url.params["project_id"] == "p-0"
            if "card_id" in request.url.params:
                offset = int(request.url.params["offset"])
                return httpx.Response(
                    200,
                    json={
                        "ok": True,
                        "data": [state] if offset == 0 else [],
                        "card_id": "c-0000",
                        "limit": 100,
                        "offset": offset,
                        "next_offset": 100 if offset == 0 else None,
                    },
                )
            assert "limit" not in request.url.params
            return ok([state])
        if path == "/resources" and request.method == "POST":
            body = json.loads(request.content)
            assert body == {
                "project_id": "p-0",
                "type": "file",
                "label": "Report",
                "metadata": {"creator": ["Ada", "Grace"]},
            }
            return httpx.Response(201, json={"ok": True, "data": state})
        if path == "/resources/r-1" and request.method == "GET":
            return ok(state)
        if path == "/resources/r-1" and request.method == "PATCH":
            body = json.loads(request.content)
            assert body == {"metadata": {"creator": []}}
            state["metadata"] = {}
            return ok(state)
        if path == "/resources/r-1" and request.method == "DELETE":
            return ok({"resource_id": "r-1"})
        if path == "/cards/c-0000/resources":
            body = json.loads(request.content)
            assert body == {"project_id": "p-0", "resource_id": "r-1"}
            return ok(
                {
                    "card_id": "c-0000",
                    "resource_id": "r-1",
                    "changed": True,
                    "outcome": "attached" if request.method == "POST" else "detached",
                }
            )
        raise AssertionError(path)

    with holder_for(respond) as holder:
        project = holder.projects.get("p-0")
        resource = project.resources.create(
            type="file", label="Report", metadata={"creator": ["Ada", "Grace"]}
        )
        assert isinstance(resource, Resource)
        assert resource.metadata["creator"] == ["Ada", "Grace"]
        assert resource.referenced_by_cards[0].link_kinds == ("attachment",)
        assert project.resources[0].id == resource.id
        card = project.cards.get("c-0000")
        assert card.attach(resource) is True
        assert card.attachments[0].id == "r-1"
        assert len(list(card.attachments)) == 1
        assert card.detach(resource) is True
        assert resource.update(metadata={"creator": []}) is resource
        assert resource.metadata == {}
        resource.delete()
    assert sum(r.method == "DELETE" for r in requests) == 2


def test_project_scope_rejects_foreign_resources_and_locations():
    def respond(request):
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        if request.url.path == "/resources/r-1":
            return ok(resource_data("p-1"))
        if request.url.path == "/locations/l-1":
            return ok(location_data("p-1"))
        raise AssertionError(request.url.path)

    with holder_for(respond) as holder:
        project = holder.projects.get("p-0")
        with pytest.raises(APIError):
            project.resources.get("r-1")
        with pytest.raises(APIError):
            project.storage_locations.get("l-1")


def test_location_binding_preference_and_import_job():
    requests = []
    state = location_data()
    polls = 0

    def respond(request):
        nonlocal polls
        path = request.url.path
        if path == "/projects/p-0":
            return ok(project_payload())
        if path == "/projects/p-0/cards/c-0000":
            return ok(card_payload(content=True))
        requests.append(request)
        if path == "/locations" and request.method == "POST":
            return httpx.Response(201, json={"ok": True, "data": state})
        if path == "/locations" and request.method == "GET":
            return httpx.Response(
                200, json={"ok": True, "data": [state], "preferred_location_id": "l-1"}
            )
        if path == "/locations/l-1" and request.method == "GET":
            return ok(state)
        if path == "/locations/l-1/binding" and request.method == "PUT":
            assert json.loads(request.content)["values"] == {"root_path": "/store"}
            state["bound"] = True
            state["binding_preview"] = "/store"
            return ok(
                {"location_id": "l-1", "bound": True, "binding_preview": "/store"}
            )
        if path == "/locations/l-1/binding" and request.method == "DELETE":
            state["bound"] = False
            return ok({"location_id": "l-1"})
        if path == "/locations/l-1/test":
            return ok({"available": True})
        if path == "/locations/preferred":
            assert json.loads(request.content) == {
                "project_id": "p-0",
                "location_id": "l-1",
            }
            return ok({"location_id": "l-1"})
        if path == "/imports" and request.method == "POST":
            assert json.loads(request.content) == {
                "project_id": "p-0",
                "card_id": "c-0000",
                "location_id": "l-1",
                "source_path": "/daemon/report.pdf",
            }
            return httpx.Response(
                202,
                json={
                    "ok": True,
                    "data": {
                        "job_id": "j-1",
                        "status": "queued",
                        "resource_id": None,
                        "asset_id": None,
                        "duplicate_reused": False,
                        "link_created": False,
                        "error": None,
                    },
                },
            )
        if path == "/imports/j-1":
            polls += 1
            return ok(
                {
                    "job_id": "j-1",
                    "status": "completed",
                    "resource_id": "r-1",
                    "asset_id": "a-1",
                    "duplicate_reused": False,
                    "link_created": True,
                    "error": None,
                }
            )
        if path == "/resources/r-1":
            return ok(resource_data())
        raise AssertionError(path)

    with holder_for(respond) as holder:
        project = holder.projects.get("p-0")
        location = project.storage_locations.create(
            name="Local", provider="local_directory"
        )
        assert isinstance(location, StorageLocation)
        assert project.storage_locations.preferred_id == "l-1"
        assert location.bind({"root_path": "/store"}, preview="/store") is location
        assert location.bound is True
        assert location.test() is True
        location.make_preferred()
        job = project.cards.get("c-0000").import_file(Path("/daemon/report.pdf"))
        assert isinstance(job, ImportJob)
        assert job.wait(interval=0.001) is job
        assert job.status == "completed" and job.link_created is True
        assert job.resource.id == "r-1"
        assert location.unbind() is location
    assert polls == 1


def test_asset_download_streams_and_keeps_destination_on_error(tmp_path):
    requests = []

    def respond(request):
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        if request.url.path == "/resources/r-1":
            return ok(resource_data(assets=[asset_data()]))
        requests.append(request)
        if request.url.params.get("placement_id") == "place-1":
            return httpx.Response(
                409,
                json={
                    "ok": False,
                    "error": {
                        "code": "binding_required",
                        "message": "Bind storage first",
                    },
                },
            )
        return httpx.Response(
            200, content=b"report", headers={"content-type": "application/pdf"}
        )

    with holder_for(respond) as holder:
        asset = holder.projects.get("p-0").resources.get("r-1").assets[0]
        assert isinstance(asset, Asset)
        destination = tmp_path / "report.pdf"
        assert asset.download_to(destination) == destination
        assert destination.read_bytes() == b"report"
        with pytest.raises(APIError) as error:
            asset.download_to(destination, placement=asset.placements[0])
        assert error.value.status == 409
        assert destination.read_bytes() == b"report"
    assert len(requests) == 2


def test_attachment_list_rejects_wrong_card():
    def respond(request):
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        if request.url.path == "/projects/p-0/cards/c-0000":
            return ok(card_payload(content=True))
        return httpx.Response(
            200, json={"ok": True, "data": [], "card_id": "other", "next_offset": None}
        )

    with holder_for(respond) as holder, pytest.raises(ProtocolError):
        list(holder.projects.get("p-0").cards.get("c-0000").attachments)


def test_import_wait_reports_failure_and_times_out():
    def respond(request):
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        return ok(
            {
                "job_id": "j-1",
                "status": "failed",
                "resource_id": None,
                "asset_id": None,
                "duplicate_reused": False,
                "link_created": False,
                "error": "Storage unavailable",
            }
        )

    with holder_for(respond) as holder:
        project = holder.projects.get("p-0")
        job = ImportJob("j-1", "queued", None, None, False, False, None, project)
        assert job.wait(interval=0.001).status == "failed"
        assert job.error == "Storage unavailable"
        assert job.resource is None
        pending = ImportJob("j-2", "queued", None, None, False, False, None, project)
        with pytest.raises(TimeoutError):
            pending.wait(timeout=0)


def test_location_update_and_delete_use_scoped_object():
    state = location_data()
    writes = []

    def respond(request):
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        if request.method == "GET":
            return ok(state)
        writes.append(request)
        if request.method == "PATCH":
            assert json.loads(request.content) == {
                "name": "Archive",
                "configuration": {"prefix": "old"},
            }
            state.update(name="Archive", configuration={"prefix": "old"})
            return ok(state)
        return ok({"location_id": "l-1"})

    with holder_for(respond) as holder:
        location = holder.projects.get("p-0").storage_locations.get("l-1")
        assert (
            location.update(name="Archive", configuration={"prefix": "old"}) is location
        )
        assert location.name == "Archive"
        location.delete()
    assert [r.method for r in writes] == ["PATCH", "DELETE"]


def test_download_error_leaves_existing_file_untouched(tmp_path):
    class BrokenStream(httpx.SyncByteStream):
        def __iter__(self):
            yield b"new"
            raise httpx.ReadError("Connection lost")

    def respond(request):
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        if request.url.path == "/resources/r-1":
            return ok(resource_data(assets=[asset_data()]))
        return httpx.Response(200, stream=BrokenStream())

    destination = tmp_path / "report.pdf"
    destination.write_bytes(b"old")
    with holder_for(respond) as holder:
        asset = holder.projects.get("p-0").resources.get("r-1").assets[0]
        from holder.exceptions import ConnectionError

        with pytest.raises(ConnectionError):
            asset.download_to(destination)
    assert destination.read_bytes() == b"old"
    assert list(tmp_path.iterdir()) == [destination]


def test_download_unauthorized_uses_the_high_level_error(tmp_path):
    def respond(request):
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        if request.url.path == "/resources/r-1":
            return ok(resource_data(assets=[asset_data()]))
        return httpx.Response(
            401,
            json={
                "ok": False,
                "error": {"code": "unauthorized", "message": "Bad token"},
            },
        )

    with holder_for(respond) as holder:
        asset = holder.projects.get("p-0").resources.get("r-1").assets[0]
        with pytest.raises(AuthenticationError):
            asset.download_to(tmp_path / "report.pdf")
    assert list(tmp_path.iterdir()) == []
