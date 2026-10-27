from enum import StrEnum


class GetProjectsProjectIdCardsCardIdBacklinksScope(StrEnum):
    ALL_AVAILABLE = "all_available"
    EXTERNAL = "external"
    LOCAL = "local"

    def __str__(self) -> str:
        return str(self.value)
