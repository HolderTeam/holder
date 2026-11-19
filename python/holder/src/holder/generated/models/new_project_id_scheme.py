from enum import StrEnum


class NewProjectIdScheme(StrEnum):
    UUID4 = "uuid4"
    UUID7 = "uuid7"

    def __str__(self) -> str:
        return str(self.value)
