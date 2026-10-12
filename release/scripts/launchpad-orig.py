#!/usr/bin/env python3
"""Provide the upstream tarball for a quilt source package uploaded to the Holder PPA.

Launchpad keeps one <source>_<version>.orig.tar.gz per source package and version, shared by every
series and Debian revision, and rejects an upload whose tarball differs from the one it has. So:

- If the PPA already has the tarball for this version (from the other series, or an earlier
  revision), download it, check that it holds exactly the files of the directory in this commit,
  and report "reused": the upload must then leave it out (debuild -sd).
- Otherwise build it with git archive, stamping files with the commit's time rather than the
  current time, so that every job builds the same file, and report "built".

    launchpad-orig.py --source holder-core --version 0.2.5 --directory core --output ../holder-core_0.2.5.orig.tar.gz
"""
import argparse
import gzip
import io
import json
import subprocess
import sys
import tarfile
import urllib.parse
import urllib.request

ARCHIVE = "https://api.launchpad.net/1.0/~holderteam/+archive/ubuntu/holder"


def get_json(url):
    with urllib.request.urlopen(url, timeout=60) as response:
        return json.load(response)


def published_orig_url(source, version, request=get_json):
    """The URL of the PPA's tarball for this source and upstream version, or None."""
    name = f"{source}_{version}.orig.tar.gz"
    query = urllib.parse.urlencode({"ws.op": "getPublishedSources", "source_name": source,
                                    "exact_match": "true", "ws.size": "100"})
    for entry in request(f"{ARCHIVE}?{query}")["entries"]:
        if not entry["source_package_version"].startswith(version + "-"):
            continue
        for url in request(entry["self_link"] + "?ws.op=sourceFileUrls"):
            if urllib.parse.unquote(url).endswith("/" + name):
                return url
    return None


def git_archive(directory, prefix, mtime=None):
    command = ["git", "archive", "--format=tar", f"--prefix={prefix}"]
    if mtime:
        command.append(f"--mtime={mtime}")
    return subprocess.run(command + [f"HEAD:{directory}"], check=True, capture_output=True).stdout


def contents(tar_bytes):
    """Each member's name, type, mode and data: what a tarball holds, ignoring timestamps."""
    found = {}
    with tarfile.open(fileobj=io.BytesIO(tar_bytes)) as archive:
        for member in archive.getmembers():
            data = archive.extractfile(member).read() if member.isfile() else member.linkname
            found[member.name.rstrip("/")] = (member.type, member.mode, data)
    return found


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--source", required=True)
    parser.add_argument("--version", required=True)
    parser.add_argument("--directory", required=True, help="the package's directory in the repository")
    parser.add_argument("--output", required=True)
    args = parser.parse_args(argv)
    prefix = f"{args.source}-{args.version}/"
    url = published_orig_url(args.source, args.version)
    if url:
        with urllib.request.urlopen(url, timeout=300) as response:
            published = response.read()
        expected = contents(git_archive(args.directory, prefix))
        if contents(gzip.decompress(published)) != expected:
            print(f"The PPA's {url.rsplit('/', 1)[-1]} does not match {args.directory}/ in this commit.", file=sys.stderr)
            print("A different upstream tree needs a new upstream version.", file=sys.stderr)
            return 1
        open(args.output, "wb").write(published)
        print(f"Reusing the PPA's tarball: {url}")
        print("reused")
        return 0
    commit_time = subprocess.run(["git", "log", "-1", "--format=%cI", "HEAD"], check=True,
                                 capture_output=True, text=True).stdout.strip()
    open(args.output, "wb").write(gzip.compress(git_archive(args.directory, prefix, commit_time), mtime=0))
    print(f"Built {args.output} from {args.directory}/, stamped {commit_time}")
    print("built")
    return 0


if __name__ == "__main__":
    sys.exit(main())
