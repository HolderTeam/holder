"""The C generator on Holder's contract.

tests/golden/ holds the expected output. After an intended change, rewrite it with
    python -m holder_contract generate --output tests/golden
and review the difference. tests/c/ compiles that output with a stand-in for core and runs it.
"""

import shutil
import subprocess
from pathlib import Path

import pytest

from holder_contract.extract import extract
from holder_contract.generate import Unsupported, camel, generate, snake

CONTRACT = Path(__file__).resolve().parents[1]
TESTS = CONTRACT / "tests"
STRICT = ["-Wall", "-Wextra", "-Wpedantic", "-Werror"]
SANITIZE = ["-fsanitize=address,undefined", "-fno-sanitize-recover=all"]


def holder_ir():
    return extract(CONTRACT, "interface")


def test_generated_files_match_the_golden_copies():
    for name, text in generate(holder_ir()).items():
        assert text == (TESTS / "golden" / name).read_text(encoding="utf-8"), name


def test_every_generated_file_names_its_source():
    for name, text in generate(holder_ir()).items():
        assert "Generated from contract/interface/cards.py" in "\n".join(text.splitlines()[:2]), name


def test_unsupported_parts_of_the_contract_stop_generation():
    with pytest.raises(Unsupported, match="enums"):
        generate(extract(TESTS / "fixtures", "sample"))


@pytest.mark.parametrize("name, expected", [("CardListItem", "card_list_item"), ("Card", "card"),
                                            ("NotFound", "not_found")])
def test_snake_case(name, expected):
    assert snake(name) == expected


def test_camel_case():
    assert camel("card_id_asc") == "CardIdAsc"


@pytest.mark.skipif(not (shutil.which("gcc") and shutil.which("g++")), reason="needs gcc and g++")
def test_generated_code_compiles_and_runs(tmp_path):
    for name, text in generate(holder_ir()).items():
        (tmp_path / name).write_text(text, encoding="utf-8")

    def run(*command):
        subprocess.run(command, check=True, cwd=tmp_path, capture_output=True, text=True)

    run("gcc", "-std=c17", *STRICT, "-fsyntax-only", "-x", "c", "holder_api.h")
    run("g++", "-std=c++20", *STRICT, "-fsyntax-only", "-x", "c++", "holder_api.h")
    run("g++", "-std=c++20", *STRICT, *SANITIZE, "-I.", "-c", "holder_api_glue.cpp", "-o", "glue.o")
    run("g++", "-std=c++20", *STRICT, *SANITIZE, "-I.", "-c", str(TESTS / "c" / "stub_core.cpp"), "-o", "stub.o")
    run("gcc", "-std=c17", *STRICT, *SANITIZE, "-I.", "-c", str(TESTS / "c" / "exercise.c"), "-o", "exercise.o")
    run("g++", *SANITIZE, "glue.o", "stub.o", "exercise.o", "-o", "exercise")
    result = subprocess.run([str(tmp_path / "exercise")], capture_output=True, text=True, check=True)
    assert result.stdout == (TESTS / "c" / "exercise.expected").read_text(encoding="utf-8")
