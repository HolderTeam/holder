"""Generate Android's typed Project bridge from the contract and its C ABI."""

from __future__ import annotations

from dataclasses import dataclass

from .android import PROJECT_JNI_PATH, PROJECT_KOTLIN_PATH, _field_type, _record_jni_lines, _record_kotlin
from .android_inputs import InputField, generate_input_record
from .android_ir import ContractView, ListType, LiteralType, NamedType, NullableType, ScalarType
from .android_templates import TEMPLATES

HEADER = ("// Generated from contract/interface/projects/ids.py, projects/models.py, projects/operations.py, maintenance.py and git.py by holder_contract.\n"
          "// Do not edit; run make -C contract update.\n")


@dataclass(frozen=True)
class RecordConversion:
    constructor: str
    record_lines: str
    construction: str
    release_refs: str


class ProjectBridgeGenerator:
    """Validate contract shapes and render the Project Kotlin and JNI templates."""

    def __init__(self, contract: ContractView) -> None:
        self.contract = contract

    def generate(self) -> dict[str, str]:
        self._validate_contract()
        from .android_recovery import generate_recovery

        recovery_record, recovery_jni = generate_recovery(self.contract)
        new_project_record, new_project_fields = generate_input_record(self.contract, "NewProject")
        project = self._typed_fields("Project")
        listing = self._typed_fields("Project", "ProjectListItem")
        sync = self._typed_fields("SyncState")
        return {
            PROJECT_KOTLIN_PATH: self._render_kotlin(project, listing, sync, new_project_record, recovery_record),
            PROJECT_JNI_PATH: self._render_jni(project, listing, sync, new_project_fields, recovery_jni),
        }

    def _validate_contract(self) -> None:
        self._validate_listing()
        self._validate_privacy_mode()
        self._validate_project_lookup()
        self._validate_project_writes()
        self._validate_project_update()
        self._validate_encryption_check()
        self._validate_sync_state()
        self._validate_pull()
        self._validate_git_operations()
        self._validate_push_fields()
        self._validate_remote_fields()
        self._validate_due_sync_result()

    def _validate_listing(self) -> None:
        operation = self.contract.operation("list_projects")
        if [p.name for p in operation.parameters] != [
            "name", "updated_after", "updated_before", "order", "limit", "offset", "include_card_counts"
        ] or not isinstance(operation.parameter("limit").type, ScalarType) or \
                operation.parameter("limit").type.le != 1000 or \
                not isinstance(operation.parameter("order").type, NullableType) or \
                not isinstance(operation.parameter("order").type.value, LiteralType) or \
                "updated_at_desc" not in operation.parameter("order").type.value.values:
            raise ValueError("Android list_projects parameters need a generator update")
        if operation.returns != ListType(kind="list", items=NamedType(name="ProjectListItem")):
            raise ValueError("Android list_projects needs list[ProjectListItem]")

    def _validate_privacy_mode(self) -> None:
        mode = self.contract.alias("PrivacyMode")
        if not isinstance(mode, LiteralType) or not all(isinstance(value, str) for value in mode.values):
            raise ValueError("Android project privacy mode needs string literals")

    def _validate_project_lookup(self) -> None:
        lookup = self.contract.operation("get_project")
        if [p.name for p in lookup.parameters] != ["project_id"] or lookup.returns != NamedType(name="Project"):
            raise ValueError("Android get_project needs a Project result")

    def _validate_project_writes(self) -> None:
        for name, parameters in {
            "create_project": ["project"],
            "update_project": ["project_id", "update"],
            "delete_project": ["project_id"],
        }.items():
            operation = self.contract.operation(name)
            if [p.name for p in operation.parameters] != parameters or operation.returns != NamedType(name="Project"):
                raise ValueError(f"Android {name} parameters or return need a generator update")
        if self.contract.operation("create_project").parameter("project").type != NamedType(name="NewProject"):
            raise ValueError("Android create_project request needs NewProject")

    def _validate_project_update(self) -> None:
        fields = self.contract.record("ProjectUpdate").fields
        if [field.name for field in fields] != ["name", "git_remote_url", "git_provider"] or \
                self.contract.operation("update_project").parameter("update").type != NamedType(name="ProjectUpdate"):
            raise ValueError("Android ProjectUpdate fields need a generator update")
        remote = fields[1]
        if not remote.omittable or remote.type != NullableType(kind="nullable", value=NamedType(name="GitRemoteUrl")):
            raise ValueError("Android project remote update needs nullable, omittable GitRemoteUrl")

    def _validate_encryption_check(self) -> None:
        operation = self.contract.operation("check_project_encryption")
        if [p.name for p in operation.parameters] != ["project_id"] or \
                operation.returns != NamedType(name="ProjectEncryptionCheck"):
            raise ValueError("Android check_project_encryption parameters or return need a generator update")
        check = self.contract.record("ProjectEncryptionCheck")
        safety = self.contract.record("EncryptionSafetyCheck")
        if [f.name for f in check.fields] != ["project_id", "privacy_mode", "check"] or \
                check.field("project_id").type != NamedType(name="ProjectId") or \
                check.field("privacy_mode").type != NamedType(name="PrivacyMode") or \
                check.field("check").type != NamedType(name="EncryptionSafetyCheck") or \
                [f.name for f in safety.fields] != ["ok", "checked_files", "unsafe_files", "unsafe_paths", "message"] or \
                safety.field("ok").type != ScalarType(kind="bool") or \
                safety.field("checked_files").type != ScalarType(kind="int32", ge=0) or \
                safety.field("unsafe_files").type != ScalarType(kind="int32", ge=0) or \
                safety.field("unsafe_paths").type != ListType(kind="list", items=ScalarType(kind="string")) or \
                safety.field("message").type != ScalarType(kind="string"):
            raise ValueError("Android encryption check fields need a generator update")

    def _validate_sync_state(self) -> None:
        operation = self.contract.operation("get_project_sync_state")
        if [p.name for p in operation.parameters] != ["project_id"] or operation.returns != NamedType(name="SyncState"):
            raise ValueError("Android get_project_sync_state parameters or return need a generator update")

    def _validate_pull(self) -> None:
        operation = self.contract.operation("pull_project")
        fields = self.contract.record("PullPhase").fields
        if [p.name for p in operation.parameters] != ["project_id"] or \
                operation.returns != NamedType(name="SyncResult") or \
                self.contract.record("SyncResult").field("pull").type != NamedType(name="PullPhase") or \
                [f.name for f in fields] != ["attempted", "status", "conflicts_resolved", "hidden_cards_repaired",
                                              "error_message"] or \
                self.contract.record("PullPhase").field("attempted").type != ScalarType(kind="bool") or \
                self.contract.record("PullPhase").field("status").type != LiteralType(
                    kind="literal", values=["not_attempted", "succeeded", "remote_unset", "failed"]) or \
                self.contract.record("PullPhase").field("error_message").type != NullableType(
                    kind="nullable", value=ScalarType(kind="string")):
            raise ValueError("Android pull_project or PullPhase needs a generator update")

    def _validate_git_operations(self) -> None:
        for name, parameters in {
            "push_project": ["project_id", "options"],
            "sync_project": ["project_id", "options"],
            "test_project_remote": ["project_id", "remote"],
        }.items():
            operation = self.contract.operation(name)
            expected = "RemoteTestResult" if name == "test_project_remote" else "SyncResult"
            if [p.name for p in operation.parameters] != parameters or operation.returns != NamedType(name=expected):
                raise ValueError(f"Android {name} parameters or return need a generator update")
    def _validate_push_fields(self) -> None:
        if [field.name for field in self.contract.record("PushPhase").fields] != [
            "attempted", "status", "ahead_count", "behind_count", "local_head_commit", "error_message"
        ] or [field.name for field in self.contract.record("PushOptions").fields] != [
            "branch", "set_upstream"
        ]:
            raise ValueError("Android project push or remote test fields need a generator update")

    def _validate_remote_fields(self) -> None:
        if [field.name for field in self.contract.record("RemoteTestResult").fields] != [
            "project_id", "status", "remote_url", "remote_has_head", "error_message"
        ] or [field.name for field in self.contract.record("RemoteTest").fields] != ["remote_url"]:
            raise ValueError("Android project push or remote test fields need a generator update")

    def _validate_due_sync_result(self) -> None:
        operation = self.contract.operation("sync_project_if_due")
        if [p.name for p in operation.parameters] != [
            "project_id", "push_interval_seconds", "pull_interval_seconds"
        ] or operation.returns != NamedType(name="DueSyncResult"):
            raise ValueError("Android sync_project_if_due parameters or return need a generator update")
        for result in ("SyncResult", "DueSyncResult"):
            record = self.contract.record(result)
            if record.field("pull").type != NamedType(name="PullPhase") or \
                    record.field("push").type != NamedType(name="PushPhase"):
                raise ValueError(f"Android {result} phase fields need a generator update")

    def _typed_fields(self, *record_names: str) -> list[tuple[str, str, str, bool]]:
        return [(field.name, *_field_type(field, self.contract,
                allow_literal=field.name == "privacy_mode"))
                for name in record_names for field in self.contract.record(name).fields]

    def _render_kotlin(self, project: list[tuple[str, str, str, bool]],
                       listing: list[tuple[str, str, str, bool]],
                       sync: list[tuple[str, str, str, bool]], new_project_record: str,
                       recovery_record: str) -> str:
        return HEADER + TEMPLATES.get_template("android_projects.kt.j2").render(
            project_list_record=_record_kotlin("ContractProjectListItem", listing),
            project_record=_record_kotlin("ContractProject", project),
            sync_record=_record_kotlin("ContractSyncState", sync),
            new_project_record=new_project_record,
            recovery_record=recovery_record,
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
                    sync: list[tuple[str, str, str, bool]], new_project_fields: list[InputField],
                    recovery_jni: str) -> str:
        item = self._record_conversion(listing, "project", "    ", {"privacy_mode": "mode"})
        project_result = self._record_conversion(project, "project", "  ", {"privacy_mode": "mode"})
        sync_result = self._record_conversion(sync, "value", "  ")
        mode = self.contract.alias("PrivacyMode").values
        return HEADER + TEMPLATES.get_template("android_projects.cpp.j2").render(
            mode_cases=self._enum_cases(mode, "HOLDER_PROJECT_PRIVACY_MODE"),
            check_mode_cases=self._enum_cases(mode, "HOLDER_PROJECT_ENCRYPTION_CHECK_PRIVACY_MODE"),
            pull_status_cases=self._enum_cases(
                self.contract.record("PullPhase").field("status").type.values, "HOLDER_PULL_PHASE_STATUS"),
            push_status_cases=self._enum_cases(self.contract.alias("PushStatus").values, "HOLDER_PUSH_PHASE_STATUS"),
            remote_test_status_cases=self._enum_cases(
                self.contract.record("RemoteTestResult").field("status").type.values,
                "HOLDER_REMOTE_TEST_RESULT_STATUS"),
            item_constructor=item.constructor, record_lines=item.record_lines,
            item_construction=item.construction, release_refs=item.release_refs,
            project_constructor=project_result.constructor, project_record_lines=project_result.record_lines,
            project_construction=project_result.construction, project_release_refs=project_result.release_refs,
            sync_constructor=sync_result.constructor, sync_record_lines=sync_result.record_lines,
            sync_construction=sync_result.construction, sync_release_refs=sync_result.release_refs,
            new_project_fields=new_project_fields,
            recovery_jni=recovery_jni,
        )


def generate_projects(contract: ContractView) -> dict[str, str]:
    """Entry point used by the Android generator."""
    return ProjectBridgeGenerator(contract).generate()
