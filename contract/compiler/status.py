"""Reserved error codes shared by the compiler and native interface backends."""

from typing import NamedTuple


class BaseErrorCode(NamedTuple):
    name: str
    description: str


# The C API's base status codes. Contract error kinds may not reuse them.
BASE_ERROR_CODES = {
    4000: BaseErrorCode("INVALID_ARGUMENT", "an invalid argument"),
    5000: BaseErrorCode("RUNTIME", "an unexpected failure"),
    5001: BaseErrorCode("ALLOCATION", "an allocation failure"),
}
