from enum import StrEnum


class AiRunnerCreateRequestKind(StrEnum):
    OLLAMA = "ollama"

    def __str__(self) -> str:
        return str(self.value)
