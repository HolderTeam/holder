"""Shared Jinja environment for Android's generated Kotlin and JNI sources."""

from pathlib import Path

from jinja2 import Environment, FileSystemLoader, StrictUndefined

TEMPLATES = Environment(
    loader=FileSystemLoader(Path(__file__).with_name("templates")),
    undefined=StrictUndefined,
    autoescape=False,
    trim_blocks=False,
    keep_trailing_newline=True,
    newline_sequence="\n",
)
