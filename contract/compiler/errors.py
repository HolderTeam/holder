"""Diagnostics produced while compiling contract declarations."""

from .rules import Problem


class ContractError(Exception):
    def __init__(self, problems: list[Problem]):
        self.problems = problems
        super().__init__("\n".join(str(problem) for problem in problems))
