from enum import StrEnum


class TrashFilter(StrEnum):
    AI_MESSAGE = "ai_message"
    ALL = "all"
    CARD = "card"

    def __str__(self) -> str:
        return str(self.value)
