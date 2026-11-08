"""Prepare product-version metadata, then delegate to the existing build backend."""
import importlib.util
from pathlib import Path

PACKAGE = Path(__file__).resolve().parents[1]
shared = PACKAGE.parent / '_build_support.py'
path = shared if shared.is_file() else PACKAGE / '_build_support.py'
spec = importlib.util.spec_from_file_location('_holder_build_support', path)
support = importlib.util.module_from_spec(spec)
spec.loader.exec_module(support)

globals().update(support.backend_hooks(PACKAGE, 'scikit_build_core.build'))
