from enum import StrEnum


class DeleteCardsCardIdChildren(StrEnum):
    PROMOTE = "promote"
    TRASH_SUBTREE = "trash_subtree"

    def __str__(self) -> str:
        return str(self.value)
