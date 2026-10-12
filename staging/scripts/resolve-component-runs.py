#!/usr/bin/env python3
"""Select successful, unexpired component artifacts and their source revisions.

The backend comes from a Daemon CI run. The desktop comes from a Desktop / CI run in the same
repository, built from the same apps/desktop/ source as the backend's commit: the two commits
may differ (Desktop / CI runs on main only when the desktop changes), but the desktop's Git tree
must not. Without a desktop run ID, the newest such run is chosen."""
import argparse
import json
import os
import subprocess

DESKTOP_PATH = ("apps", "desktop")
ARTIFACTS = {
    "linux": ("holder-desktop-linux-x86_64", "holder-daemon-linux-x86_64"),
    "windows": ("holder-desktop-windows", "holder-daemon-windows-backend"),
    "macos": ("holder-desktop-macos", "holder-daemon-macos-backend"),
}


def api(path, **query):
    command = ["gh", "api", "--method", "GET", path]
    for name, value in query.items():
        command += ["-f", f"{name}={value}"]
    return json.loads(subprocess.check_output(command, text=True))


def candidate_runs(repository, workflow, branch, provided, request):
    if provided and not provided.isdecimal():
        raise ValueError("Provided run ID must be numeric")
    if provided:
        return [request(f"repos/{repository}/actions/runs/{provided}")]
    return request(f"repos/{repository}/actions/workflows/{workflow}/runs",
                   branch=branch, status="success", per_page="30")["workflow_runs"]


def has_artifact(repository, run, artifact, request):
    if run["status"] != "completed" or run["conclusion"] != "success":
        return False
    artifacts = request(f"repos/{repository}/actions/runs/{run['id']}/artifacts")["artifacts"]
    return any(item["name"] == artifact and not item["expired"] for item in artifacts)


def resolve(repository, workflow, branch, artifact, provided="", request=api, accept=None):
    """Return (run ID, head commit) of the newest successful run with the artifact that accept allows."""
    for run in candidate_runs(repository, workflow, branch, provided, request):
        if has_artifact(repository, run, artifact, request) and (accept is None or accept(run["head_sha"])):
            return str(run["id"]), run["head_sha"]
    if provided:
        raise ValueError(f"Run {repository}#{provided} is not a successful run with an unexpired {artifact} artifact"
                         + (" built from the backend's desktop source" if accept else ""))
    raise ValueError(f"No successful {workflow} run on {repository}@{branch} has an unexpired {artifact} artifact"
                     + (" built from the backend's desktop source" if accept else ""))


def subtree(repository, commit, path, request=api):
    """The Git tree SHA of path (a tuple of names) at commit."""
    tree = request(f"repos/{repository}/commits/{commit}")["commit"]["tree"]["sha"]
    for name in path:
        entries = request(f"repos/{repository}/git/trees/{tree}")["tree"]
        tree = next((entry["sha"] for entry in entries if entry["path"] == name and entry["type"] == "tree"), None)
        if tree is None:
            raise ValueError(f"{repository}@{commit} has no {'/'.join(path)}")
    return tree


def backend_workflow(repository):
    """Core publishes backends from its integration run; HolderTeam/holder from its own CI."""
    return "daemon-integration.yml" if repository == "HolderTeam/holder-core" else "daemon-ci.yml"


def select(platform, build_type, repository, branch, desktop_run="", backend_run="", request=api):
    suffix = "-release" if build_type == "Release" else ""
    desktop_artifact, backend_artifact = (name + suffix for name in ARTIFACTS[platform])
    backend_id, backend_sha = resolve(repository, backend_workflow(repository), branch, backend_artifact,
                                      backend_run, request)
    tree = subtree(repository, backend_sha, DESKTOP_PATH, request)
    desktop_id, desktop_sha = resolve(repository, "desktop-ci.yml", branch, desktop_artifact, desktop_run, request,
                                      accept=lambda sha: sha == backend_sha or subtree(repository, sha, DESKTOP_PATH, request) == tree)
    return {"backend_run_id": backend_id, "backend_commit": backend_sha, "backend_artifact": backend_artifact,
            "desktop_run_id": desktop_id, "desktop_commit": desktop_sha, "desktop_artifact": desktop_artifact,
            "desktop_tree": tree}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--platform", choices=tuple(ARTIFACTS), required=True)
    parser.add_argument("--build-type", choices=("Release", "RelWithDebInfo"), default="RelWithDebInfo")
    args = parser.parse_args()
    selected = select(args.platform, args.build_type, os.environ["BACKEND_REPOSITORY"],
                      os.environ.get("BACKEND_REF") or "main", os.environ.get("DESKTOP_RUN_ID_INPUT", ""),
                      os.environ.get("BACKEND_RUN_ID_INPUT", ""))
    with open(os.environ["GITHUB_OUTPUT"], "a") as output:
        for key, value in selected.items():
            output.write(f"{key}={value}\n")
    repository = os.environ["BACKEND_REPOSITORY"]
    summary = (f"## Resolved component runs\n\n"
               f"- Backend: [{repository}#{selected['backend_run_id']}](https://github.com/{repository}/actions/runs/{selected['backend_run_id']}) at `{selected['backend_commit']}`\n"
               f"- Desktop: [{repository}#{selected['desktop_run_id']}](https://github.com/{repository}/actions/runs/{selected['desktop_run_id']}) at `{selected['desktop_commit']}` (apps/desktop tree `{selected['desktop_tree']}`)\n")
    print(summary)
    if os.environ.get("GITHUB_STEP_SUMMARY"):
        with open(os.environ["GITHUB_STEP_SUMMARY"], "a") as output:
            output.write(summary)


if __name__ == "__main__":
    main()
