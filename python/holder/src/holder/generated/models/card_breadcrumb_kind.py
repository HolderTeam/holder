from enum import StrEnum


class CardBreadcrumbKind(StrEnum):
    CARD = "card"
    PROJECT = "project"

    def __str__(self) -> str:
        return str(self.value)
