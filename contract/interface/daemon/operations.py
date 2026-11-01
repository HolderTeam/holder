"""Daemon operations."""

from typing import Literal

from authoring import Contract

from .models import AliveResponse, ByeResponse, HealthResponse


def register(contract: Contract) -> None:
    @contract.route("GET", "/alive", core=False, public=True, raw_media_type="application/json")
    def get_alive() -> AliveResponse:
        """Liveness check returning JSON, unauthenticated."""


    @contract.route("GET", "/ping", core=False, public=True, raw_media_type="text/plain")
    def get_ping() -> Literal["pong"]:
        """Liveness check returning plain text, unauthenticated."""


    @contract.route("GET", "/health", core=False, raw_media_type="application/json")
    def get_health() -> HealthResponse:
        """Health check."""


    @contract.route("POST", "/bye", core=False, raw_media_type="application/json")
    def post_bye() -> ByeResponse:
        """Say that this client is leaving.

        A hint to a daemon started with --idle-exit: it shortens the idle wait only while nothing else happens.
        Running work, a stream, or another request keeps the daemon running. Other daemon modes ignore it.
        """
