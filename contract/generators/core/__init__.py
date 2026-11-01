"""Core's generated C and C++ interfaces.

Three files come out:

- holder_api.h: the C declarations every consumer of core's C API sees.
- Contract.h: the C++ declarations core implements, one function per operation, in holder::contract.
- ContractCApi.cpp: the C entry points. Each checks its arguments against the contract, calls
  core's C++ function, converts the result into the caller's arena, and turns error kinds into
  status codes.

Only what the contract uses so far is supported; anything else stops generation with a message,
and is added when the first declaration needs it.
"""

from __future__ import annotations

from compiler.ir import ContractView

from .c_header import generate_c_header
from .cpp_header import generate_cpp_header
from .glue import generate_glue
from .types import CoreTypes

CORE_PATHS = {"holder_api.h": "core/include/holder/holder_api.h", "Contract.h": "core/src/api/Contract.h",
              "ContractCApi.cpp": "core/src/api/ContractCApi.cpp"}


def generate_core(contract: ContractView) -> dict[str, str]:
    """The generated files, by name."""
    types = CoreTypes(contract)
    return {"holder_api.h": generate_c_header(types), "Contract.h": generate_cpp_header(types),
            "ContractCApi.cpp": generate_glue(types)}
