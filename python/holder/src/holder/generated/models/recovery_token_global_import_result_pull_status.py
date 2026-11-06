from enum import StrEnum


class RecoveryTokenGlobalImportResultPullStatus(StrEnum):
    FAILED = "failed"
    NOT_ATTEMPTED = "not_attempted"
    SUCCEEDED = "succeeded"

    def __str__(self) -> str:
        return str(self.value)
