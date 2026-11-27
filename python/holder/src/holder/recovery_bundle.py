"""Write portable recovery bundles without opening the live database."""

from __future__ import annotations

import hashlib
import json
import re
from collections.abc import Iterable, Iterator
from contextlib import contextmanager
from pathlib import Path
from shutil import copyfileobj
from tempfile import TemporaryDirectory
from typing import TYPE_CHECKING, NamedTuple
from zipfile import BadZipFile, ZipFile

from .exceptions import APIError, ProtocolError

if TYPE_CHECKING:
    from os import PathLike

    from .client import Holder


class _Asset(NamedTuple):
    resource_id: str
    asset_id: str
    size: int
    digest: str
    placements: tuple[str, ...]


def _snapshot_assets(lines: Iterable[str], project_id: str) -> dict[str, _Asset]:
    assets: dict[str, _Asset] = {}
    try:
        for line in lines:
            if not line.rstrip("\r\n"):
                continue
            row = json.loads(line)
            card = row["card"]
            if (
                row["format"] != "holder-recovery-snapshot"
                or row["version"] != 1
                or card["project_id"] != project_id
            ):
                raise ValueError
            for resource in card["resources"]:
                for item in resource["assets"]:
                    resource_id = resource["resource_id"]
                    asset_id = item["asset_id"]
                    digest = item["plaintext_sha256"]
                    size = item["byte_size"]
                    placements = tuple(p["placement_id"] for p in item["placements"])
                    if (
                        not all(
                            isinstance(value, str) and value
                            for value in (resource_id, asset_id)
                        )
                        or not re.fullmatch(r"[0-9a-f]{64}", digest)
                        or type(size) is not int
                        or size < 0
                        or not all(isinstance(p, str) and p for p in placements)
                    ):
                        raise ValueError
                    asset = _Asset(resource_id, asset_id, size, digest, placements)
                    existing = assets.get(digest)
                    if existing is not None and existing.size != size:
                        raise ValueError
                    if existing is None or (not existing.placements and placements):
                        assets[digest] = asset
    except (KeyError, TypeError, ValueError) as error:
        raise ProtocolError(
            "Invalid asset metadata in the recovery snapshot"
        ) from error
    return assets


def _download_verified_asset(session: Holder, asset: _Asset, path: Path) -> None:
    for placement in asset.placements:
        try:
            with path.open("wb") as output:
                size = session.download_asset(
                    asset.resource_id, asset.asset_id, output, placement_id=placement
                )
        except APIError as error:
            if error.status not in {404, 409, 422, 502, 503, 507}:
                raise
            continue
        if size == asset.size:
            with path.open("rb") as contents:
                if hashlib.file_digest(contents, "sha256").hexdigest() == asset.digest:
                    return
    raise ProtocolError(
        f"Could not recover the captured bytes for asset {asset.asset_id}"
    )


def _download_recovery_bundle(
    session: Holder, project_id: str, destination: str | PathLike[str]
) -> None:
    target = Path(destination)
    with TemporaryDirectory(prefix=".holder-recovery-", dir=target.parent) as temporary:
        directory = Path(temporary)
        snapshot = directory / "snapshot.jsonl"
        with snapshot.open("xb") as output:
            session._write_recovery_snapshot(project_id, output)
        with snapshot.open(encoding="utf-8") as lines:
            assets = _snapshot_assets(lines, project_id)
        archive_path = directory / "bundle.zip"
        payload = directory / "asset"
        with ZipFile(archive_path, "w") as archive:
            archive.writestr(
                "manifest.json",
                json.dumps(
                    {
                        "format": "holder-recovery-bundle",
                        "version": 1,
                        "project_id": project_id,
                    }
                ),
            )
            archive.write(snapshot, "snapshot.jsonl")
            for asset in assets.values():
                _download_verified_asset(session, asset, payload)
                archive.write(payload, f"assets/{asset.digest}")
                payload.unlink()
        archive_path.replace(target)


@contextmanager
def _extract_recovery_bundle(
    source: str | PathLike[str],
) -> Iterator[tuple[Path, Path]]:
    with TemporaryDirectory(prefix="holder-recovery-") as temporary:
        directory = Path(temporary)
        try:
            with ZipFile(source) as archive:
                entries = archive.namelist()
                if len(set(entries)) != len(entries):
                    raise ValueError("Duplicate recovery bundle entries")
                if archive.getinfo("manifest.json").file_size > 4096:
                    raise ValueError("Recovery bundle metadata exceeds its size limit")
                manifest = json.loads(archive.read("manifest.json"))
                project_id = manifest["project_id"]
                if (
                    manifest["format"] != "holder-recovery-bundle"
                    or manifest["version"] != 1
                    or not isinstance(project_id, str)
                    or not project_id
                ):
                    raise ValueError("Unsupported recovery bundle")
                snapshot = directory / "snapshot.jsonl"
                with (
                    archive.open("snapshot.jsonl") as contents,
                    snapshot.open("xb") as output,
                ):
                    copyfileobj(contents, output)
                with snapshot.open(encoding="utf-8") as lines:
                    assets = _snapshot_assets(lines, project_id)
                expected = {"manifest.json", "snapshot.jsonl"} | {
                    f"assets/{digest}" for digest in assets
                }
                if set(entries) != expected:
                    raise ValueError(
                        "Recovery bundle asset entries do not match the snapshot"
                    )
                for digest, asset in assets.items():
                    member = archive.getinfo(f"assets/{digest}")
                    if member.file_size != asset.size:
                        raise ValueError(
                            "Recovery bundle asset size does not match the snapshot"
                        )
                    # Use the validated digest as the local filename. Never extract paths from the ZIP.
                    with (
                        archive.open(member) as contents,
                        (directory / digest).open("xb") as output,
                    ):
                        copyfileobj(contents, output)
        except (
            BadZipFile,
            KeyError,
            ProtocolError,
            TypeError,
            UnicodeDecodeError,
        ) as error:
            raise ValueError("Invalid Holder recovery bundle") from error
        yield snapshot, directory
