"""Document operations."""

from typing import Annotated

from fastapi import Path

from authoring import Bytes, Contract, JsonValue
from authoring.routes import ErrorResponse


def register(contract: Contract) -> None:
    @contract.route("GET", "/openapi.yaml", core=False, public=True, raw_media_type="application/yaml",
                    raw_schema={})
    def get_openapi_yaml() -> str:
        """Download the OpenAPI specification."""


    @contract.route("GET", "/openapi.json", core=False, public=True, raw_media_type="application/json",
                    raw_schema={})
    def get_openapi_json() -> JsonValue:
        """Download the OpenAPI specification as JSON."""


    @contract.route("GET", "/ai_catalog.yaml", core=False, public=True, raw_media_type="application/yaml",
                    raw_schema={},
                    responses={404: {"model": ErrorResponse, "description": "Catalog not found."}})
    def get_ai_catalog_yaml() -> str:
        """Download the canonical AI catalog config."""


    @contract.route("GET", "/ai_catalog.json", core=False, public=True, raw_media_type="application/json",
                    raw_schema={},
                    responses={404: {"model": ErrorResponse, "description": "Catalog not found."}})
    def get_ai_catalog_json() -> JsonValue:
        """Download the canonical AI catalog config as JSON."""


    @contract.route("GET", "/git_providers.yaml", core=False, public=True, raw_media_type="application/yaml",
                    raw_schema={},
                    responses={404: {"model": ErrorResponse, "description": "Config not found."}})
    def get_git_providers_yaml() -> str:
        """Download the Git provider UI and runtime config."""


    @contract.route("GET", "/git_providers.json", core=False, public=True, raw_media_type="application/json",
                    raw_schema={},
                    responses={404: {"model": ErrorResponse, "description": "Config not found."}})
    def get_git_providers_json() -> JsonValue:
        """Download the Git provider UI and runtime config as JSON."""


    @contract.route("GET", "/docs", core=False, public=True, raw_media_type="text/html",
                    responses={404: {"model": ErrorResponse, "description": "Documentation assets not installed."}})
    def get_docs() -> str:
        """Browse the API documentation."""


    @contract.route("GET", "/docs/{file}", core=False, public=True, raw_media_type="*/*",
                    responses={404: {"model": ErrorResponse, "description": "Documentation file not found."}})
    def get_docs_file(file: Annotated[str, Path()]) -> Bytes:
        """Download an API documentation file with its media type."""
