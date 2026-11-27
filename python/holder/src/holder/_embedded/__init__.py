"""Private location for the native extension and schema in platform wheels."""

import os
import sys
from pathlib import Path

_dll_directory = None
if sys.platform == "win32":
    bundled_libraries = Path(__file__).parent / ".libs"
    if bundled_libraries.is_dir():
        # Keep the loader handle alive for the lifetime of the extension.
        _dll_directory = os.add_dll_directory(str(bundled_libraries))
