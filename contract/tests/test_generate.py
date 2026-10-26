"""The C generator on Holder's contract.

Core holds the generated files (see holder_contract.generate.CORE_PATHS). After an intended change
to the contract or the generator, rewrite them with make -C contract update and review the
difference. tests/c/ compiles the output with a stand-in for core and runs it.
"""

import shutil
import subprocess
from pathlib import Path

import pytest

from holder_contract.extract import extract
from holder_contract.generate import CORE_PATHS, Unsupported, camel, generate, snake

CONTRACT = Path(__file__).resolve().parents[1]
TESTS = CONTRACT / "tests"
STRICT = ["-Wall", "-Wextra", "-Wpedantic", "-Werror"]
SANITIZE = ["-fsanitize=address,undefined", "-fno-sanitize-recover=all"]


def holder_ir():
    return extract(CONTRACT, "interface")


def test_cores_generated_files_are_up_to_date():
    for name, text in generate(holder_ir()).items():
        assert text == (CONTRACT.parent / CORE_PATHS[name]).read_text(encoding="utf-8"), CORE_PATHS[name]


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
    # Laid out as in core: holder.h (the real one) includes the generated holder_api.h.
    (tmp_path / "holder").mkdir()
    shutil.copy(CONTRACT.parent / "core/include/holder/holder.h", tmp_path / "holder" / "holder.h")
    for name, text in generate(holder_ir()).items():
        (tmp_path / ("holder" if name == "holder_api.h" else ".") / name).write_text(text, encoding="utf-8")

    def run(*command):
        subprocess.run(command, check=True, cwd=tmp_path, capture_output=True, text=True)

    (tmp_path / "include.c").write_text('#include "holder/holder.h"\n')
    run("gcc", "-std=c17", *STRICT, "-fsyntax-only", "-I.", "include.c")
    run("g++", "-std=c++20", *STRICT, "-fsyntax-only", "-I.", "-x", "c++", "include.c")
    run("g++", "-std=c++20", *STRICT, *SANITIZE, "-I.", "-c", "ContractGlue.cpp", "-o", "glue.o")
    run("g++", "-std=c++20", *STRICT, *SANITIZE, "-I.", "-c", str(TESTS / "c" / "stub_core.cpp"), "-o", "stub.o")
    run("gcc", "-std=c17", *STRICT, *SANITIZE, "-I.", "-c", str(TESTS / "c" / "exercise.c"), "-o", "exercise.o")
    run("g++", *SANITIZE, "glue.o", "stub.o", "exercise.o", "-o", "exercise")
    result = subprocess.run([str(tmp_path / "exercise")], capture_output=True, text=True, check=True)
    assert result.stdout == (TESTS / "c" / "exercise.expected").read_text(encoding="utf-8")
