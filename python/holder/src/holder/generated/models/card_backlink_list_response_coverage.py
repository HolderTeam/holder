from enum import StrEnum


class CardBacklinkListResponseCoverage(StrEnum):
    AVAILABLE_PROJECTS = "available_projects"
    TARGET_PROJECT = "target_project"

    def __str__(self) -> str:
        return str(self.value)
