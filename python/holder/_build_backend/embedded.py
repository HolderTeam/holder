"""Optionally package the staged Kit extension as a native Holder wheel."""

import os
import sys
from pathlib import Path
from sysconfig import get_config_var

from hatchling.builders.hooks.plugin.interface import BuildHookInterface


class EmbeddedWheelHook(BuildHookInterface):
    def initialize(self, version, build_data):
        stage = os.environ.get("HOLDER_PYTHON_EMBEDDED_STAGE")
        if not stage or version != "standard":
            return
        if sys.platform != "linux" or sys.implementation.name != "cpython":
            raise ValueError("The embedded wheel build currently supports Linux CPython")

        package = Path(stage).resolve() / "holderkit"
        extension = "_native" + get_config_var("EXT_SUFFIX")
        for name in (extension, "_schema.sql", "_core_build.json"):
            artifact = package / name
            if not artifact.is_file():
                raise FileNotFoundError(f"Missing staged embedded artifact: {artifact}")
            build_data["force_include"][str(artifact)] = f"holder/_embedded/{name}"

        build_data["infer_tag"] = True
        build_data["pure_python"] = False
