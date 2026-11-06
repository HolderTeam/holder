from enum import StrEnum


class LinkTargetType(StrEnum):
    AI_MESSAGE = "ai_message"
    AI_THREAD = "ai_thread"
    CARD = "card"
    RESOURCE = "resource"

    def __str__(self) -> str:
        return str(self.value)
