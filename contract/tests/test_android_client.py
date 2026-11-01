"""The generic Android client: every core operation, from type rules alone."""

from copy import deepcopy
from pathlib import Path

from compiler.ir import ContractView
from generators.android.client import JNI_PATH, KOTLIN_DIR, generate_android_client
from holder_contract.extract import extract

CONTRACT = Path(__file__).resolve().parents[1]


def holder_ir():
    return extract(CONTRACT, "interface")


def client(ir):
    return generate_android_client(ContractView.model_validate(ir))


def test_every_core_operation_has_a_kotlin_function_and_a_jni_entry_point():
    ir = holder_ir()
    files = client(ir)
    kotlin = "\n".join(text for path, text in files.items() if path.endswith(".kt"))
    for op in ir["operations"]:
        if op["daemon_only"]:
            continue
        name = op["name"].split("_")[0] + "".join(part.title() for part in op["name"].split("_")[1:])
        assert f"external fun {name}(" in kotlin, op["name"]
        assert f"_{name}(JNIEnv*" in files[JNI_PATH], op["name"]


def test_a_new_operation_needs_no_generator_change():
    ir = holder_ir()
    get_card = next(op for op in ir["operations"] if op["name"] == "get_card")
    ir["operations"].append({**deepcopy(get_card), "name": "get_card_again"})
    files = client(ir)
    assert "external fun getCardAgain(" in files[f"{KOTLIN_DIR}/Cards.kt"]
    assert "Java_team_holder_android_contract_Cards_getCardAgain(" in files[JNI_PATH]
    assert "holder_get_card_again(" in files[JNI_PATH]


def test_a_new_enum_choice_reaches_kotlin_and_jni():
    ir = holder_ir()
    move = next(r for r in ir["records"] if r["name"] == "CardMove")
    intent = next(f for f in move["fields"] if f["name"] == "intent")
    intent["type"]["values"].append("to_middle")
    files = client(ir)
    assert 'TO_MIDDLE("to_middle")' in files[f"{KOTLIN_DIR}/Cards.kt"]


def test_error_kinds_become_exceptions_with_their_codes():
    files = client(holder_ir())
    assert "class NotFoundException(message: String) : ContractException(4040, message)" in files[f"{KOTLIN_DIR}/Errors.kt"]
    assert "class InvalidMoveException(message: String) : ContractException(4220, message)" in files[f"{KOTLIN_DIR}/Cards.kt"]
    assert 'case 4040: type = "team/holder/android/contract/NotFoundException"; break;' in files[JNI_PATH]


def test_runtime_operations_follow_their_context_modes():
    files = client(holder_ir())
    runtime = files[f"{KOTLIN_DIR}/Runtime.kt"]
    jni = files[JNI_PATH]
    assert "external fun version(): CoreVersion" in runtime
    assert "external fun contextOpen(\n        dataDir: String,\n        schemaSql: String? = null,\n    ): Long" in runtime
    assert "external fun contextDestroy(\n        contextHandle: Long,\n    )" in runtime
    assert "external fun databaseRebuild(\n        dataDir: String," in runtime
    assert "Runtime_contextOpen(JNIEnv* env, jobject, jstring a_data_dir" in jni
    assert "holder_context_open(\n      p_data_dir,\n      p_schema_sql,\n      &opened, &error);" in jni
    assert "Runtime_contextDestroy(JNIEnv*, jobject, jlong context_handle)" in jni
    assert "holder_database_rebuild(\n      p_data_dir," in jni
