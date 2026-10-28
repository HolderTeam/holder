from enum import StrEnum


class TrashItemType(StrEnum):
    AI_MESSAGE = "ai_message"
    CARD = "card"

    def __str__(self) -> str:
        return str(self.value)
