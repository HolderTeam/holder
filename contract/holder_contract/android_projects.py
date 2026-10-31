"""Generate Android's typed Project bridge from the contract and its C ABI."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any

from .android import PROJECT_JNI_PATH, PROJECT_KOTLIN_PATH, _field_type, _record_jni_lines, _record_kotlin
from .android_templates import TEMPLATES

HEADER = ("// Generated from contract/interface/projects.py, maintenance.py, git.py and app.py by holder_contract.\n"
          "// Do not edit; run make -C contract update.\n")


@dataclass(frozen=True)
class RecordConversion:
    constructor: str
    record_lines: str
    construction: str
    release_refs: str


class ProjectBridgeGenerator:
    """Validate contract shapes and render the Project Kotlin and JNI templates."""

    def __init__(self, ir: dict[str, Any], records: dict[str, dict[str, Any]],
                 aliases: dict[str, dict[str, Any]]) -> None:
        self.operations = {operation["name"]: operation for operation in ir["operations"]}
        self.records = records
        self.aliases = aliases

    def generate(self) -> dict[str, str]:
        self._validate_contract()
        project = self._typed_fields("Project")
        listing = self._typed_fields("Project", "ProjectListItem")
        sync = self._typed_fields("SyncState")
        return {
            PROJECT_KOTLIN_PATH: self._render_kotlin(project, listing, sync),
            PROJECT_JNI_PATH: self._render_jni(project, listing, sync),
        }

    def _validate_contract(self) -> None:
        self._validate_listing()
        self._validate_privacy_mode()
        self._validate_project_lookup()
        self._validate_project_writes()
        self._validate_new_project()
        self._validate_project_update()
        self._validate_encryption_check()
        self._validate_sync_state()
        self._validate_pull()
        self._validate_git_operations()
        self._validate_push_fields()
        self._validate_remote_fields()
        self._validate_sync_result()

    def _validate_listing(self) -> None:
        operation = self.operations["list_projects"]
        if [p["name"] for p in operation["parameters"]] != [
            "name", "updated_after", "updated_before", "order", "limit", "offset", "include_card_counts"
        ] or operation["parameters"][4]["type"].get("le") != 1000 or \
                "updated_at_desc" not in operation["parameters"][3]["type"]["value"]["values"]:
            raise ValueError("Android list_projects parameters need a generator update")
        if operation["returns"] != {"kind": "list", "items": {"kind": "named", "name": "ProjectListItem"}}:
            raise ValueError("Android list_projects needs list[ProjectListItem]")

    def _validate_privacy_mode(self) -> None:
        mode = self.aliases["PrivacyMode"]
        if mode["kind"] != "literal" or not all(isinstance(value, str) for value in mode["values"]):
            raise ValueError("Android project privacy mode needs string literals")

    def _validate_project_lookup(self) -> None:
        lookup = self.operations["get_project"]
        if [p["name"] for p in lookup["parameters"]] != ["project_id"] or \
                lookup["returns"] != {"kind": "named", "name": "Project"}:
            raise ValueError("Android get_project needs a Project result")

    def _validate_project_writes(self) -> None:
        for name, parameters in {
            "create_project": ["project"],
            "update_project": ["project_id", "update"],
            "delete_project": ["project_id"],
        }.items():
            operation = self.operations[name]
            if [p["name"] for p in operation["parameters"]] != parameters or operation["returns"] != {
                "kind": "named", "name": "Project"
            }:
                raise ValueError(f"Android {name} parameters or return need a generator update")

    def _validate_new_project(self) -> None:
        fields = self.records["NewProject"]["fields"]
        if [field["name"] for field in fields] != [
            "project_id", "name", "root_path", "git_remote_url", "git_provider", "privacy_mode",
            "created_at", "updated_at"
        ] or self.operations["create_project"]["parameters"][0]["type"] != {
            "kind": "named", "name": "NewProject"
        } or fields[5].get("default") != "encrypted_git":
            raise ValueError("Android NewProject fields need a generator update")

    def _validate_project_update(self) -> None:
        fields = self.records["ProjectUpdate"]["fields"]
        if [field["name"] for field in fields] != ["name", "git_remote_url", "git_provider"] or \
                self.operations["update_project"]["parameters"][1]["type"] != {
                    "kind": "named", "name": "ProjectUpdate"
                }:
            raise ValueError("Android ProjectUpdate fields need a generator update")
        remote = fields[1]
        if not remote.get("omittable") or remote["type"] != {
            "kind": "nullable", "value": {"kind": "named", "name": "GitRemoteUrl"}
        }:
            raise ValueError("Android project remote update needs nullable, omittable GitRemoteUrl")

    def _validate_encryption_check(self) -> None:
        operation = self.operations["check_project_encryption"]
        if [p["name"] for p in operation["parameters"]] != ["project_id"] or operation["returns"] != {
            "kind": "named", "name": "ProjectEncryptionCheck"
        }:
            raise ValueError("Android check_project_encryption parameters or return need a generator update")
        check = self.records["ProjectEncryptionCheck"]["fields"]
        safety = self.records["EncryptionSafetyCheck"]["fields"]
        if [(f["name"], f["type"]) for f in check] != [
            ("project_id", {"kind": "named", "name": "ProjectId"}),
            ("privacy_mode", {"kind": "named", "name": "PrivacyMode"}),
            ("check", {"kind": "named", "name": "EncryptionSafetyCheck"}),
        ] or [(f["name"], f["type"]) for f in safety] != [
            ("ok", {"kind": "bool"}),
            ("checked_files", {"kind": "int32", "ge": 0}),
            ("unsafe_files", {"kind": "int32", "ge": 0}),
            ("unsafe_paths", {"kind": "list", "items": {"kind": "string"}}),
            ("message", {"kind": "string"}),
        ]:
            raise ValueError("Android encryption check fields need a generator update")

    def _validate_sync_state(self) -> None:
        operation = self.operations["get_project_sync_state"]
        if [p["name"] for p in operation["parameters"]] != ["project_id"] or operation["returns"] != {
            "kind": "named", "name": "SyncState"
        }:
            raise ValueError("Android get_project_sync_state parameters or return need a generator update")

    def _validate_pull(self) -> None:
        operation = self.operations["pull_project"]
        fields = self.records["PullPhase"]["fields"]
        if [p["name"] for p in operation["parameters"]] != ["project_id"] or operation["returns"] != {
            "kind": "named", "name": "SyncResult"
        } or not any(f["name"] == "pull" and f["type"] == {"kind": "named", "name": "PullPhase"}
                     for f in self.records["SyncResult"]["fields"]) or [(f["name"], f["type"]) for f in fields] != [
            ("attempted", {"kind": "bool"}),
            ("status", {"kind": "literal", "values": ["not_attempted", "succeeded", "remote_unset", "failed"]}),
            ("conflicts_resolved", {"kind": "int32"}),
            ("hidden_cards_repaired", {"kind": "int32"}),
            ("error_message", {"kind": "nullable", "value": {"kind": "string"}}),
        ]:
            raise ValueError("Android pull_project or PullPhase needs a generator update")

    def _validate_git_operations(self) -> None:
        for name, parameters in {
            "push_project": ["project_id", "options"],
            "sync_project": ["project_id", "options"],
            "test_project_remote": ["project_id", "remote"],
        }.items():
            operation = self.operations[name]
            expected = "RemoteTestResult" if name == "test_project_remote" else "SyncResult"
            if [p["name"] for p in operation["parameters"]] != parameters or operation["returns"] != {
                "kind": "named", "name": expected
            }:
                raise ValueError(f"Android {name} parameters or return need a generator update")
    def _validate_push_fields(self) -> None:
        if [field["name"] for field in self.records["PushPhase"]["fields"]] != [
            "attempted", "status", "ahead_count", "behind_count", "local_head_commit", "error_message"
        ] or [field["name"] for field in self.records["PushOptions"]["fields"]] != [
            "branch", "set_upstream"
        ]:
            raise ValueError("Android project push or remote test fields need a generator update")

    def _validate_remote_fields(self) -> None:
        if [field["name"] for field in self.records["RemoteTestResult"]["fields"]] != [
            "project_id", "status", "remote_url", "remote_has_head", "error_message"
        ] or [field["name"] for field in self.records["RemoteTest"]["fields"]] != ["remote_url"]:
            raise ValueError("Android project push or remote test fields need a generator update")

    def _validate_sync_result(self) -> None:
        if [field["name"] for field in self.records["SyncResult"]["fields"]] != [
            "project_id", "status", "remote_url", "pull", "push"
        ]:
            raise ValueError("Android SyncResult fields need a generator update")

    def _typed_fields(self, *record_names: str) -> list[tuple[str, str, str, bool]]:
        return [(field["name"], *_field_type(field, self.aliases,
                allow_literal=field["name"] == "privacy_mode"))
                for name in record_names for field in self.records[name]["fields"]]

    def _render_kotlin(self, project: list[tuple[str, str, str, bool]],
                       listing: list[tuple[str, str, str, bool]],
                       sync: list[tuple[str, str, str, bool]]) -> str:
        return HEADER + TEMPLATES.get_template("android_projects.kt.j2").render(
            project_list_record=_record_kotlin("ContractProjectListItem", listing),
            project_record=_record_kotlin("ContractProject", project),
            sync_record=_record_kotlin("ContractSyncState", sync),
        )

    @staticmethod
    def _enum_cases(values: list[str], prefix: str) -> str:
        return "".join(f'      case {prefix}_{value.upper()}: return "{value}";\n' for value in values)

    @staticmethod
    def _record_conversion(fields: list[tuple[str, str, str, bool]], source: str,
                           indent: str, overrides: dict[str, str] | None = None) -> RecordConversion:
        lines, arguments, refs = _record_jni_lines(fields, source, overrides, indent=indent)
        descriptor = "(" + "".join(signature for _, _, signature, _ in fields) + ")V"
        constructor = f'  jmethodID item_ctor = env->GetMethodID(item_type, "<init>", "{descriptor}");\n'
        construction = f"{indent}jobject item = env->NewObject(item_type, item_ctor, {', '.join(arguments)});\n"
        release = "".join(f"{indent}if ({ref}) env->DeleteLocalRef({ref});\n" for ref in refs)
        return RecordConversion(constructor, "".join(lines), construction, release)

    def _render_jni(self, project: list[tuple[str, str, str, bool]],
                    listing: list[tuple[str, str, str, bool]],
                    sync: list[tuple[str, str, str, bool]]) -> str:
        item = self._record_conversion(listing, "project", "    ", {"privacy_mode": "mode"})
        project_result = self._record_conversion(project, "project", "  ", {"privacy_mode": "mode"})
        sync_result = self._record_conversion(sync, "value", "  ")
        mode = self.aliases["PrivacyMode"]["values"]
        return HEADER + TEMPLATES.get_template("android_projects.cpp.j2").render(
            mode_cases=self._enum_cases(mode, "HOLDER_PROJECT_PRIVACY_MODE"),
            check_mode_cases=self._enum_cases(mode, "HOLDER_PROJECT_ENCRYPTION_CHECK_PRIVACY_MODE"),
            pull_status_cases=self._enum_cases(
                self.records["PullPhase"]["fields"][1]["type"]["values"], "HOLDER_PULL_PHASE_STATUS"),
            push_status_cases=self._enum_cases(self.aliases["PushStatus"]["values"], "HOLDER_PUSH_PHASE_STATUS"),
            remote_test_status_cases=self._enum_cases(
                self.records["RemoteTestResult"]["fields"][1]["type"]["values"],
                "HOLDER_REMOTE_TEST_RESULT_STATUS"),
            item_constructor=item.constructor, record_lines=item.record_lines,
            item_construction=item.construction, release_refs=item.release_refs,
            project_constructor=project_result.constructor, project_record_lines=project_result.record_lines,
            project_construction=project_result.construction, project_release_refs=project_result.release_refs,
            sync_constructor=sync_result.constructor, sync_record_lines=sync_result.record_lines,
            sync_construction=sync_result.construction, sync_release_refs=sync_result.release_refs,
        )


def generate_projects(ir: dict[str, Any], records: dict[str, dict[str, Any]],
                      aliases: dict[str, dict[str, Any]]) -> dict[str, str]:
    """Entry point used by the Android generator."""
    return ProjectBridgeGenerator(ir, records, aliases).generate()
