"""The C generator on Holder's contract.

Core holds the generated files (see generators.core.CORE_PATHS). After an intended change
to the contract or the generator, rewrite them with make -C contract update and review the
difference. tests/c/ compiles the output with a stand-in for core and runs it.
"""

import shutil
import subprocess
from pathlib import Path

import pytest

from compiler.ir import ContractView
from generators.common import Unsupported
from generators.core.abi import snake
from generators.core.types import CoreTypes, camel
from generators.daemon.contract_json import generate_codec
from holder_contract.extract import extract
from holder_contract.generate import CORE_PATHS, generate

CONTRACT = Path(__file__).resolve().parents[1]
TESTS = CONTRACT / "tests"
STRICT = ["-Wall", "-Wextra", "-Wpedantic", "-Werror"]
SANITIZE = ["-fsanitize=address,undefined", "-fno-sanitize-recover=all"]
NLOHMANN = [Path(p) for p in ("/usr/include/nlohmann/json.hpp", "/usr/local/include/nlohmann/json.hpp")]


def holder_ir():
    return extract(CONTRACT, "interface")


def test_cores_generated_files_are_up_to_date():
    for name, text in generate(holder_ir()).items():
        assert text == (CONTRACT.parent / CORE_PATHS[name]).read_text(encoding="utf-8"), CORE_PATHS[name]


def test_every_generated_file_names_its_source():
    for name, text in generate(holder_ir()).items():
        header = "\n".join(text.splitlines()[:2])
        assert "contract/interface/cards/operations.py" in header, name
        assert "contract/interface/cards/models.py" in header, name


def test_unsupported_parts_of_the_contract_stop_generation():
    with pytest.raises(Unsupported, match="enums"):
        generate(extract(TESTS / "fixtures", "sample"))


@pytest.mark.parametrize("constraint, message", [({"pattern": "[a-z]+"}, "patterns"),
                                                 ({"kind": "float64", "ge": 0}, "bounds on a float64")])
def test_constraints_the_glue_cannot_check_stop_generation(constraint, message):
    ir = holder_ir()
    get_card = next(op for op in ir["operations"] if op["name"] == "get_card")
    get_card["parameters"][0]["type"] = {"kind": "string", **constraint}
    with pytest.raises(Unsupported, match=message):
        generate(ir)


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
    run("g++", "-std=c++20", *STRICT, *SANITIZE, "-I.", "-c", "ContractCApi.cpp", "-o", "glue.o")
    run("g++", "-std=c++20", *STRICT, *SANITIZE, "-I.", "-c", str(TESTS / "c" / "stub_core.cpp"), "-o", "stub.o")
    run("gcc", "-std=c17", *STRICT, *SANITIZE, "-I.", "-c", str(TESTS / "c" / "exercise.c"), "-o", "exercise.o")
    run("g++", *SANITIZE, "glue.o", "stub.o", "exercise.o", "-o", "exercise")
    result = subprocess.run([str(tmp_path / "exercise")], capture_output=True, text=True, check=True)
    assert result.stdout == (TESTS / "c" / "exercise.expected").read_text(encoding="utf-8")


# ---- partial updates and choices in records ---------------------------------------------------


def with_partial_update(ir):
    """Holder's contract plus change_card, whose CardChange keeps, sets or clears a card's parent,
    and says how with a choice of literals."""
    ir["records"].append({"name": "CardChange", "source": "interface/cards/models.py", "doc": "A change to a card.",
                          "fields": [{"name": "intent", "doc": "How to place it.",
                                      "type": {"kind": "literal", "values": ["into", "to_end"]}},
                                     {"name": "parent_card_id", "doc": "Null makes it a root card.", "omittable": True,
                                      "type": {"kind": "nullable", "value": {"kind": "named", "name": "CardId"}}}]})
    ir["operations"].append({"name": "change_card", "source": "interface/app.py", "summary": "Change a card.",
                             "description": None, "errors": [], "daemon_only": False,
                             "parameters": [{"name": "card_id", "type": {"kind": "named", "name": "CardId"}},
                                            {"name": "change", "type": {"kind": "named", "name": "CardChange"}}],
                             "returns": {"kind": "named", "name": "Card"}})
    return ir


# What core sees: the change_card the tests below link, which names the change in the card's title.
CHANGE_CARD = r"""
#include "Contract.h"

namespace holder::contract {

Card change_card(holder_context&, std::string card_id, const CardChange& change) {
  Card card{};
  card.card_id = card_id;
  card.title = std::string(change.intent == CardChangeIntent::Into ? "into, " : "to end, ") +
               (!change.parent_card_id ? "keep" : !*change.parent_card_id ? "clear" : "set " + **change.parent_card_id);
  return card;
}

}  // namespace holder::contract
"""


def test_a_partial_update_is_a_change_in_c_and_cpp():
    files = generate(with_partial_update(holder_ir()))
    assert "typedef enum holder_change {" in files["holder_api.h"]
    assert "    holder_string_change parent_card_id; /* Null makes it a root card. Zero keeps it. */" in files["holder_api.h"]
    assert "using Change = std::optional<std::optional<T>>;" in files["Contract.h"]
    assert "  Change<std::string> parent_card_id;  ///< Null makes it a root card." in files["Contract.h"]
    # A choice of literals in a record is an enum named for the record and the field.
    assert "    HOLDER_CARD_CHANGE_INTENT_INTO = 1," in files["holder_api.h"]
    assert "    holder_card_change_intent intent; /* How to place it. */" in files["holder_api.h"]
    assert "enum class CardChangeIntent { Into, ToEnd };" in files["Contract.h"]


def test_a_partial_update_of_anything_but_text_stops_generation():
    ir = with_partial_update(holder_ir())
    ir["records"][-1]["fields"][1]["type"]["value"] = {"kind": "int64"}
    with pytest.raises(Unsupported, match="a partial update of a int64, which only strings have so far"):
        generate(ir)


def test_a_partial_update_in_a_result_stops_generation():
    ir = with_partial_update(holder_ir())
    ir["operations"][-1]["returns"] = {"kind": "named", "name": "CardChange"}
    with pytest.raises(Unsupported, match="CardChange.parent_card_id: a partial update in a result"):
        generate(ir)


def test_a_choice_in_a_result_is_converted_to_its_c_enum():
    ir = with_partial_update(holder_ir())
    ir["records"][-1]["fields"].pop(1)
    ir["operations"][-1]["returns"] = {"kind": "named", "name": "CardChange"}
    glue = generate(ir)["ContractCApi.cpp"]
    assert "holder_card_change_intent to_c_enum(contract::CardChangeIntent value) {" in glue
    assert "  case contract::CardChangeIntent::ToEnd: return HOLDER_CARD_CHANGE_INTENT_TO_END;" in glue
    assert "  out.intent = to_c_enum(value.intent);" in glue


def test_an_optional_choice_in_a_record_stops_generation():
    ir = with_partial_update(holder_ir())
    ir["records"][-1]["fields"][0]["omittable"] = True
    with pytest.raises(Unsupported, match="CardChange.intent: an optional choice of literals in a record"):
        generate(ir)


@pytest.mark.skipif(not (shutil.which("gcc") and shutil.which("g++")), reason="needs gcc and g++")
def test_a_partial_update_keeps_sets_or_clears_through_the_c_api(tmp_path):
    (tmp_path / "holder").mkdir()
    shutil.copy(CONTRACT.parent / "core/include/holder/holder.h", tmp_path / "holder" / "holder.h")
    for name, text in generate(with_partial_update(holder_ir())).items():
        (tmp_path / ("holder" if name == "holder_api.h" else ".") / name).write_text(text, encoding="utf-8")
    (tmp_path / "change_card.cpp").write_text(CHANGE_CARD, encoding="utf-8")
    (tmp_path / "main.c").write_text(r"""
#include "holder/holder.h"

#include <stdio.h>

holder_context* stub_context_new(void);
void stub_context_free(holder_context* context);

static void change(holder_context* context, holder_arena* arena, int intent, int kind, const char* value) {
    holder_card_change card_change = {0};
    card_change.intent = (holder_card_change_intent)intent;
    card_change.parent_card_id.change = (holder_change)kind;
    card_change.parent_card_id.value = value;
    const holder_card* card = NULL;
    holder_error* error = NULL;
    if (holder_change_card(context, "c1", &card_change, arena, &card, &error) == HOLDER_OK) {
        printf("%s\n", card->title);
    } else {
        printf("%s\n", holder_error_message(error));
        holder_error_destroy(error);
    }
}

int main(void) {
    holder_context* context = stub_context_new();
    holder_arena* arena = holder_arena_new();
    change(context, arena, HOLDER_CARD_CHANGE_INTENT_INTO, HOLDER_KEEP, "ignored");
    change(context, arena, HOLDER_CARD_CHANGE_INTENT_TO_END, HOLDER_SET, "c2");
    change(context, arena, HOLDER_CARD_CHANGE_INTENT_INTO, HOLDER_CLEAR, "ignored");
    change(context, arena, HOLDER_CARD_CHANGE_INTENT_INTO, HOLDER_SET, NULL);
    change(context, arena, HOLDER_CARD_CHANGE_INTENT_INTO, HOLDER_SET, "");
    change(context, arena, HOLDER_CARD_CHANGE_INTENT_INTO, 7, "c2");
    change(context, arena, 0, HOLDER_KEEP, NULL);
    holder_arena_free(arena);
    stub_context_free(context);
    return 0;
}
""", encoding="utf-8")

    def run(*command):
        subprocess.run(command, check=True, cwd=tmp_path, capture_output=True, text=True)

    run("g++", "-std=c++20", *STRICT, *SANITIZE, "-I.", "-c", "ContractCApi.cpp", "-o", "glue.o")
    run("g++", "-std=c++20", *STRICT, *SANITIZE, "-I.", "-c", str(TESTS / "c" / "stub_core.cpp"), "-o", "stub.o")
    run("g++", "-std=c++20", *STRICT, *SANITIZE, "-I.", "-c", "change_card.cpp", "-o", "change_card.o")
    run("gcc", "-std=c17", *STRICT, *SANITIZE, "-I.", "-c", "main.c", "-o", "main.o")
    run("g++", *SANITIZE, "glue.o", "stub.o", "change_card.o", "main.o", "-o", "main")
    result = subprocess.run([str(tmp_path / "main")], capture_output=True, text=True, check=True)
    assert result.stdout.splitlines() == [
        "into, keep", "to end, set c2", "into, clear",
        "card_change.parent_card_id.value must not be null",
        "card_change.parent_card_id.value must not be empty",
        "card_change.parent_card_id.change is not a holder_change value",
        "card_change.intent is not a holder_card_change_intent value"]


@pytest.mark.skipif(not shutil.which("g++") or not any(p.exists() for p in NLOHMANN),
                    reason="needs g++ and nlohmann/json")
def test_a_partial_update_keeps_sets_or_clears_through_json(tmp_path):
    ir = with_partial_update(holder_ir())
    (tmp_path / "Contract.h").write_text(generate(ir)["Contract.h"], encoding="utf-8")
    (tmp_path / "ContractJson.h").write_text(generate_codec(CoreTypes(ContractView.model_validate(ir)), "Contract.h"), encoding="utf-8")
    (tmp_path / "main.cpp").write_text(r"""
#include "ContractJson.h"

#include <iostream>

int main() {
  using holder::contract::CardChange;
  for (const char* text : {R"({"intent": "into"})", R"({"intent": "into", "parent_card_id": null})",
                           R"({"intent": "to_end", "parent_card_id": "c2"})", R"({"intent": "into", "parent_card_id": ""})",
                           R"({"intent": "into", "parent_card_id": 7})", R"({"intent": "up"})", R"({})"}) {
    try {
      const auto change = contract_json::decode<CardChange>(nlohmann::json::parse(text));
      std::cout << contract_json::encode(change).dump() << "\n";
    } catch (const std::invalid_argument& error) {
      std::cout << error.what() << "\n";
    }
  }
}
""", encoding="utf-8")
    subprocess.run(["g++", "-std=c++20", *STRICT, *SANITIZE, "-I.", "main.cpp", "-o", "main"], check=True,
                   cwd=tmp_path, capture_output=True, text=True)
    result = subprocess.run([str(tmp_path / "main")], capture_output=True, text=True, check=True)
    assert result.stdout.splitlines() == [
        '{"intent":"into"}', '{"intent":"into","parent_card_id":null}', '{"intent":"to_end","parent_card_id":"c2"}',
        "parent_card_id must not be empty", "parent_card_id must be a string",
        "intent must be one of into, to_end", "intent is required and must not be null"]


def test_runtime_operations_get_the_c_and_cpp_signatures_of_their_context_modes():
    files = generate(holder_ir())
    header, cpp, glue = files["holder_api.h"], files["Contract.h"], files["ContractCApi.cpp"]
    assert ("int holder_context_open(const char* data_dir, const char* schema_sql, holder_context** out_context, "
            "holder_error** out_error);") in header
    assert "void holder_context_destroy(holder_context* context);" in header
    assert ("int holder_version(holder_arena* arena, const holder_core_version** out_core_version, "
            "holder_error** out_error);") in header
    assert "int holder_database_rebuild(const char* data_dir, const char* schema_sql, bool dry_run, holder_arena* arena" in header
    assert "holder_context* context_open(std::string data_dir, std::optional<std::string> schema_sql);" in cpp
    assert "void context_destroy(holder_context* context) noexcept;" in cpp
    assert "CoreVersion version();" in cpp
    assert "{ contract::context_destroy(context); }" in glue
    assert "*out_context = contract::context_open(data_dir_value, schema_sql_value);" in glue
