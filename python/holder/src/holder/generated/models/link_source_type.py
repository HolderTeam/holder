from enum import StrEnum


class LinkSourceType(StrEnum):
    AI_MESSAGE = "ai_message"
    CARD = "card"

    def __str__(self) -> str:
        return str(self.value)
