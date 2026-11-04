"""Every generated output must select the contract freshness check in CI."""

import fnmatch
import os
import subprocess
from pathlib import Path

import yaml

from compiler.__main__ import repository_files
from compiler.pipeline import Compiler, ContractSource
from compiler.source import load_contract
from generators.core import generate_core


def test_generated_outputs_select_contract_ci(tmp_path):
    root = Path(__file__).resolve().parents[2]
    workflow_path = ".github/workflows/contract-check.yml"
    # BaseLoader preserves GitHub's `on` key as text instead of YAML 1.1's boolean.
    workflow = yaml.load((root / workflow_path).read_text(), Loader=yaml.BaseLoader)
    push_paths = workflow["on"]["push"]["paths"]
    select = next(step["run"] for step in workflow["jobs"]["changes"]["steps"] if step.get("id") == "select")

    source = root / "contract"
    ir = Compiler().compile(ContractSource(source, "interface"))
    outputs = repository_files(ir, load_contract(source, "interface"), generate_core(ir))
    selected = [*outputs, "contract/interface/app.py", "api/openapi-base.json", workflow_path]
    unrelated = ["README.md", "core/src/api/Cards.cpp", "daemon/src/api/GeneratedRoutes.cpp",
                 "apps/mobile/android/app/src/main/java/team/holder/android/MainActivity.kt"]

    for path in selected + unrelated:
        expected = path in selected
        assert any(fnmatch.fnmatchcase(path, pattern) for pattern in push_paths) == expected, path
        output = tmp_path / "output"
        output.write_text("")
        # Execute the actual PR selector, substituting only Git's changed-file listing.
        script = 'git() { printf "%s\\n" "$CHANGED_PATH"; }\n' + select
        subprocess.run(["bash", "-e", "-o", "pipefail", "-c", script], check=True, capture_output=True,
                       env={**os.environ, "EVENT": "pull_request", "CHANGED_PATH": path,
                            "GITHUB_OUTPUT": str(output)})
        assert output.read_text() == f"contract={str(expected).lower()}\n", path
