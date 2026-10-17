from enum import StrEnum


class GetProjectsProjectIdMilestonesWithDatesValueType(StrEnum):
    DATE = "date"
    DATETIME = "datetime"

    def __str__(self) -> str:
        return str(self.value)
