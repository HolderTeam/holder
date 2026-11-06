#!/usr/bin/env python3
"""Resolve core once, then fetch a verified immutable SDK for each build host.

A selection names either a published SDK release (resolve) or SDK archives built earlier in the
same CI run (index); fetch verifies and unpacks the archive for this host from either source.
shelf downloads this host's archive for a core/ tree from HolderTeam/holder-sdk, where main's
green builds are published, so a build whose core/ is unchanged needn't build core."""

import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import platform
import re
import shutil
import tarfile
import tempfile
import urllib.error
import urllib.request
import zipfile
import sys

sys.path.insert(0, str(Path(__file__).parent))
from sdk_dependencies import relocate_windows_packages

REPOSITORY = "HolderTeam/holder-core"
SHELF = "HolderTeam/holder-sdk"
API = f"https://api.github.com/repos/{REPOSITORY}/"


def require(condition, message):
    if not condition:
        raise ValueError(message)


def read_url(url):
    require(url.startswith("https://"), "SDK downloads require HTTPS")
    headers = {"User-Agent": "holder-core-sdk-consumer"}
    # Never send the API token to an asset host or redirect destination.
    if url.startswith(API):
        token = os.environ.get("GH_TOKEN") or os.environ.get("GITHUB_TOKEN")
        if token:
            headers["Authorization"] = f"Bearer {token}"
    with urllib.request.urlopen(urllib.request.Request(url, headers=headers), timeout=120) as response:
        return response.read()


def api(path):
    return json.loads(read_url(API + path))


def resolve(core_ref):
    if core_ref == "latest-green":
        ref = api("git/ref/tags/latest-green")["object"]
        require(ref["type"] == "commit", "latest-green must point directly to a commit")
        commit = ref["sha"]
    else:
        require(re.fullmatch(r"[0-9a-f]{40}|v[0-9]+\.[0-9]+\.[0-9]+(?:[-.][A-Za-z0-9.-]+)?", core_ref),
                "Use latest-green, a full core commit SHA, or a version tag")
        commit = api(f"commits/{core_ref}")["sha"]
    require(re.fullmatch(r"[0-9a-f]{40}", commit), "Invalid resolved core commit")
    tag = core_ref if core_ref.startswith("v") else f"sdk-{commit}"
    release = api(f"releases/tags/{tag}")
    require(not release["draft"], "SDK release is unpublished")
    if tag.startswith("sdk-"):
        indexes = [a for a in release["assets"] if a["name"] == "sdk-index.json"]
        require(len(indexes) == 1, "SDK snapshot index missing")
        index = json.loads(read_url(indexes[0]["browser_download_url"]))
        require(index["schema_version"] == 1 and index["repository"] == REPOSITORY
                and index["commit"] == commit and index["snapshot_tag"] == tag,
                "SDK snapshot index does not match the resolved commit")
        assets = index["assets"]
    else:
        assets = []
        for asset in release["assets"]:
            match = re.fullmatch(r"libholder-(.+)-(linux|macos|windows)-(x86_64|arm64)-(release|relwithdebinfo)\.(tar\.gz|zip)", asset["name"])
            if match:
                version, host, arch, config, _ = match.groups()
                require(asset.get("digest", "").startswith("sha256:"), "Tagged SDK lacks a SHA256 digest")
                assets.append({"name": asset["name"], "platform": host, "architecture": arch,
                               "build_type": {"release": "Release", "relwithdebinfo": "RelWithDebInfo"}[config],
                               "sha256": asset["digest"][7:], "size": asset["size"],
                               "url": asset["browser_download_url"]})
        index = {"version": tag[1:]}
    require(len(assets) == 6, "Expected all six core SDK configurations")
    expected = {(system, arch, config) for system, arch in
                [("linux", "x86_64"), ("macos", "arm64"), ("windows", "x86_64")]
                for config in ["Release", "RelWithDebInfo"]}
    require({(a["platform"], a["architecture"], a["build_type"]) for a in assets} == expected,
            "Incomplete or duplicate core SDK configurations")
    return {"repository": REPOSITORY, "commit": commit, "version": index["version"],
            "release_tag": tag, "assets": assets}


def host():
    systems = {"Linux": "linux", "Darwin": "macos", "Windows": "windows"}
    architectures = {"x86_64": "x86_64", "AMD64": "x86_64", "arm64": "arm64", "aarch64": "arm64"}
    return systems[platform.system()], architectures[platform.machine()]


def selected_asset(selection, system, architecture, build_type):
    matches = [a for a in selection["assets"] if (a["platform"], a["architecture"], a["build_type"])
               == (system, architecture, build_type)]
    require(len(matches) == 1, "No unique SDK for this platform, architecture and configuration")
    asset = matches[0]
    require(re.fullmatch(r"[0-9a-f]{64}", asset["sha256"]), "Invalid SDK checksum")
    require(PurePosixPath(asset["name"]).name == asset["name"], "Invalid SDK asset name")
    if selection.get("source") != "artifact":
        require(asset["url"] == f"https://github.com/{REPOSITORY}/releases/download/{selection['release_tag']}/{asset['name']}",
                "Unexpected SDK download URL")
    return asset


def unpack(archive, destination):
    def safe(name):
        path = PurePosixPath(name)
        require(not path.is_absolute() and ".." not in path.parts and "\\" not in name
                and path.parts and path.parts[0] == "libholder-sdk", "Unsafe SDK archive path")
    if archive.name.endswith(".zip"):
        with zipfile.ZipFile(archive) as package:
            for entry in package.infolist():
                safe(entry.filename)
                require((entry.external_attr >> 16) & 0o170000 != 0o120000, "SDK archive contains a symlink")
            package.extractall(destination)
    else:
        with tarfile.open(archive, "r:gz") as package:
            for entry in package.getmembers():
                safe(entry.name)
                require(entry.isfile() or entry.isdir(), "SDK archive contains a link or special file")
            package.extractall(destination, **({"filter": "data"} if hasattr(tarfile, "data_filter") else {}))


def archive_manifest(archive):
    """Read libholder-manifest.json from an SDK archive without unpacking the rest."""
    name = "libholder-sdk/libholder-manifest.json"
    if archive.name.endswith(".zip"):
        with zipfile.ZipFile(archive) as package:
            return json.loads(package.read(name))
    with tarfile.open(archive, "r:gz") as package:
        member = package.extractfile(name)
        require(member is not None, "SDK archive lacks its manifest")
        return json.loads(member.read())


def index(assets_dir, commit=None, tree=None):
    """Describe the SDK archives built earlier in this CI run as a selection that fetch accepts.

    Each platform's SDK is cached by core/'s tree on its own, so archives of the same tree can
    record different commits: whichever commit first built each one. Given the run's commit and
    core/ tree, the selection records those, and each asset keeps the commit its archive records.
    Without them, every archive must record the same commit."""
    archives = sorted(p for p in assets_dir.iterdir() if p.name.endswith((".tar.gz", ".zip")))
    require(archives, f"No SDK archives in {assets_dir}")
    assets, manifests = [], []
    for archive in archives:
        manifest = archive_manifest(archive)
        manifests.append(manifest)
        data = archive.read_bytes()
        require(re.fullmatch(r"[0-9a-f]{40}", manifest["commit"]), "Invalid SDK commit")
        assets.append({"name": archive.name, "platform": manifest["platform"],
                       "architecture": manifest["architecture"], "build_type": manifest["build_type"],
                       "commit": manifest["commit"], "size": len(data), "sha256": hashlib.sha256(data).hexdigest()})
    require(len({m["version"] for m in manifests}) == 1, "SDK archives disagree on version")
    if commit is None:
        require(len({m["commit"] for m in manifests}) == 1, "SDK archives disagree on commit")
        commit = manifests[0]["commit"]
    require(re.fullmatch(r"[0-9a-f]{40}", commit), "Invalid run commit")
    configurations = [(a["platform"], a["architecture"], a["build_type"]) for a in assets]
    require(len(set(configurations)) == len(configurations), "Duplicate SDK configurations")
    selection = {"source": "artifact", "commit": commit, "version": manifests[0]["version"], "assets": assets}
    if tree:
        selection["core_tree"] = tree
    return selection


def fetch(selection, cache, system, architecture, build_type, assets_dir=None, install_dir=None):
    # Archives are cached by commit. The unpacked SDK goes beside its archive unless install_dir
    # names a fixed place: CI uses one, because the SDK's path reaches the compiler's include
    # flags, and a path naming the commit would make every compiler cache entry miss.
    asset = selected_asset(selection, system, architecture, build_type)
    root = cache / selection["commit"] / system / architecture / build_type
    root.mkdir(parents=True, exist_ok=True)
    archive = root / asset["name"]
    if not archive.exists():
        if selection.get("source") == "artifact":
            require(assets_dir is not None, "An artifact selection needs the directory of SDK archives")
            data = (assets_dir / asset["name"]).read_bytes()
        else:
            data = read_url(asset["url"])
        require(len(data) == asset["size"] and hashlib.sha256(data).hexdigest() == asset["sha256"],
                "Downloaded SDK checksum or size mismatch")
        archive.write_bytes(data)
    require(archive.stat().st_size == asset["size"]
            and hashlib.sha256(archive.read_bytes()).hexdigest() == asset["sha256"],
            "Cached SDK checksum or size mismatch; remove the damaged archive and retry")
    with tempfile.TemporaryDirectory(dir=root) as temporary:
        unpack(archive, Path(temporary))
        sdk = Path(temporary) / "libholder-sdk"
        manifest = json.loads((sdk / "libholder-manifest.json").read_text())
        for key, expected in {"commit": asset.get("commit", selection["commit"]), "version": selection["version"],
                              "platform": system, "architecture": architecture, "build_type": build_type}.items():
            require(manifest[key] == expected, f"SDK manifest {key} mismatch")
        require((sdk / "share/holder/schema/schema.sql").is_file(), "SDK schema missing")
        require((sdk / "lib/cmake/holder/holderConfig.cmake").is_file(), "SDK CMake package missing")
        if system == "windows":
            require((sdk / "vcpkg/scripts/buildsystems/vcpkg.cmake").is_file(),
                    "Windows SDK lacks bundled development dependencies; select a newer core snapshot")
            # Normalize legacy snapshots as well as newly packaged SDKs. The
            # cached archive is preserved and rechecked against its checksum.
            relocate_windows_packages(sdk / "vcpkg/installed/x64-windows")
        target = Path(install_dir) if install_dir is not None else root / "libholder-sdk"
        if target.exists():
            shutil.rmtree(target)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.move(str(sdk), target)
    return target.resolve()


def shelf(tree, build_type, output, system, architecture):
    """Download this host's SDK for core/'s tree from holder-sdk's core-<tree> release.

    Returns the verified archive's path, or None when holder-sdk has no such release or archive
    (core changed since main's last green build) or can't be reached, so the caller builds core
    instead. An index or archive that doesn't match what it claims is an error, never a fallback."""
    require(re.fullmatch(r"[0-9a-f]{40}", tree), "Invalid core tree")
    tag = f"core-{tree}"
    base = f"https://github.com/{SHELF}/releases/download/{tag}/"
    try:
        index = json.loads(read_url(base + "sdk-index.json"))
    except urllib.error.HTTPError as error:
        if error.code != 404:
            print(f"warning: holder-sdk answered {error.code}; building core instead", file=sys.stderr)
        return None
    except urllib.error.URLError as error:
        print(f"warning: holder-sdk unreachable ({error.reason}); building core instead", file=sys.stderr)
        return None
    require(index.get("schema_version") == 1 and index.get("repository") == SHELF
            and index.get("release_tag") == tag and index.get("core_tree") == tree,
            "holder-sdk index does not match the requested core tree")
    matches = [a for a in index["assets"] if (a["platform"], a["architecture"], a["build_type"])
               == (system, architecture, build_type)]
    require(len(matches) <= 1, "Duplicate SDKs in the holder-sdk index")
    if not matches:
        return None
    asset = matches[0]
    require(PurePosixPath(asset["name"]).name == asset["name"], "Invalid SDK asset name")
    require(asset["url"] == base + asset["name"], "Unexpected SDK download URL")
    require(re.fullmatch(r"[0-9a-f]{64}", asset["sha256"]), "Invalid SDK checksum")
    data = read_url(asset["url"])
    require(len(data) == asset["size"] and hashlib.sha256(data).hexdigest() == asset["sha256"],
            "Downloaded SDK checksum or size mismatch")
    output.mkdir(parents=True, exist_ok=True)
    archive = output / asset["name"]
    archive.write_bytes(data)
    manifest = archive_manifest(archive)
    for key in ("commit", "platform", "architecture", "build_type"):
        require(manifest[key] == asset[key], f"SDK manifest {key} mismatch")
    return archive


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    resolution = commands.add_parser("resolve")
    resolution.add_argument("--core-ref", default=os.environ.get("HOLDER_CORE_REF", "latest-green"))
    resolution.add_argument("--output", type=Path, default=Path("out/core-selection.json"))
    indexing = commands.add_parser("index")
    indexing.add_argument("--assets", type=Path, required=True)
    indexing.add_argument("--output", type=Path, default=Path("out/core-selection.json"))
    indexing.add_argument("--commit", help="The commit this run builds")
    indexing.add_argument("--tree", help="core/'s Git tree in that commit")
    shelved = commands.add_parser("shelf")
    shelved.add_argument("--tree", required=True, help="core/'s Git tree")
    shelved.add_argument("--build-type", choices=["Release", "RelWithDebInfo"], default="RelWithDebInfo")
    shelved.add_argument("--output", type=Path, required=True, help="Where to put the archive")
    download = commands.add_parser("fetch")
    download.add_argument("--selection", type=Path, default=Path("out/core-selection.json"))
    download.add_argument("--cache", type=Path, default=Path(".core-sdk"))
    download.add_argument("--build-type", choices=["Release", "RelWithDebInfo"], default="RelWithDebInfo")
    download.add_argument("--github-env", action="store_true")
    download.add_argument("--expected-commit", help="Require the run-wide resolved commit")
    download.add_argument("--assets", type=Path, help="SDK archives for an artifact selection")
    download.add_argument("--install-dir", type=Path,
                          help="Unpack the SDK here instead of beside its cached archive")
    args = parser.parse_args()
    if args.command == "shelf":
        archive = shelf(args.tree, args.build_type, args.output, *host())
        print(archive if archive else f"holder-sdk has no {args.build_type} SDK for core tree {args.tree}")
        if os.environ.get("GITHUB_OUTPUT"):
            with open(os.environ["GITHUB_OUTPUT"], "a") as output:
                output.write(f"found={'true' if archive else 'false'}\n")
        return
    if args.command in ("resolve", "index"):
        selection = (resolve(args.core_ref) if args.command == "resolve"
                     else index(args.assets, args.commit, args.tree))
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(selection, indent=2) + "\n")
        print(selection["commit"])
        if os.environ.get("GITHUB_OUTPUT"):
            with open(os.environ["GITHUB_OUTPUT"], "a") as output:
                output.write(f"commit={selection['commit']}\n")
    else:
        selection = json.loads(args.selection.read_text())
        if args.expected_commit:
            require(selection["commit"] == args.expected_commit, "SDK selection differs from the run-wide core commit")
        sdk = fetch(selection, args.cache, *host(), args.build_type, args.assets, args.install_dir)
        cmake_args = ""
        if platform.system() == "Windows":
            cmake_args += (f' -DCMAKE_TOOLCHAIN_FILE="{sdk.as_posix()}/vcpkg/scripts/buildsystems/vcpkg.cmake"'
                           f' -DVCPKG_INSTALLED_DIR="{sdk.as_posix()}/vcpkg/installed"'
                           " -DVCPKG_TARGET_TRIPLET=x64-windows -DVCPKG_MANIFEST_MODE=OFF -DVCPKG_APPLOCAL_DEPS=OFF")
        if args.github_env:
            with open(os.environ["GITHUB_ENV"], "a") as output:
                output.write(f"HOLDER_CORE_SDK={sdk.as_posix()}\nCMAKE_ARGS={cmake_args}\nHOLDER_CORE_BUILD_TYPE={args.build_type}\n")
        if os.environ.get("GITHUB_OUTPUT"):
            with open(os.environ["GITHUB_OUTPUT"], "a") as output:
                output.write(f"sdk={sdk.as_posix()}\n")
        print(sdk)


if __name__ == "__main__":
    main()
