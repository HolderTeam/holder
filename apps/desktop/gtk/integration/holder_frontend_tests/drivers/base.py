from __future__ import annotations

from abc import ABC, abstractmethod
import importlib.util
import os
from pathlib import Path


def launch_environment(env: dict[str, str]) -> dict[str, str]:
    """Select at the native app launch boundary so macOS retains DYLD_*.

    Available to every driver; accessibility is opt-in via the runner's --access.
    """
    if os.environ.get('HOLDER_FRONTEND_ACCESSIBLE') != '1':
        return env
    selector = Path(__file__).resolve().parents[3] / 'scripts/accessible_runtime.py'
    spec = importlib.util.spec_from_file_location('holder_accessible_runtime', selector)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module.launch_environment(env)


class FrontendDriver(ABC):
    """Minimal cross-client driver contract used by shared Behave steps."""

    @abstractmethod
    def launch(self) -> None:
        raise NotImplementedError

    @abstractmethod
    def shutdown(self) -> None:
        raise NotImplementedError

    @abstractmethod
    def create_card(self) -> None:
        raise NotImplementedError

    @abstractmethod
    def create_project(self, name: str) -> None:
        raise NotImplementedError

    @abstractmethod
    def has_project_named(self, name: str) -> bool:
        raise NotImplementedError

    @abstractmethod
    def select_project(self, name: str) -> None:
        raise NotImplementedError

    @abstractmethod
    def has_card_titled_prefix(self, prefix: str) -> bool:
        raise NotImplementedError

    @abstractmethod
    def replace_editor_text(self, text: str) -> None:
        raise NotImplementedError

    @abstractmethod
    def save_state_is_visible(self, text: str) -> bool:
        raise NotImplementedError

    @abstractmethod
    def search_cards(self, query: str) -> None:
        raise NotImplementedError

    @abstractmethod
    def has_search_result(self, text: str) -> bool:
        raise NotImplementedError

    @abstractmethod
    def open_sidebar_card(self, text: str) -> None:
        raise NotImplementedError

    @abstractmethod
    def replace_all_in_editor(self, find_text: str, replace_text: str) -> None:
        raise NotImplementedError

    @abstractmethod
    def editor_text_contains(self, text: str) -> bool:
        raise NotImplementedError

    @abstractmethod
    def editor_text_excludes(self, text: str) -> bool:
        raise NotImplementedError

    @abstractmethod
    def open_find_replace_panel(self) -> None:
        raise NotImplementedError

    @abstractmethod
    def toggle_find_replace_panel(self) -> None:
        raise NotImplementedError

    @abstractmethod
    def find_replace_panel_is_visible(self) -> bool:
        raise NotImplementedError

    @abstractmethod
    def find_replace_panel_is_hidden(self) -> bool:
        raise NotImplementedError

    @abstractmethod
    def open_preferences(self) -> None:
        raise NotImplementedError

    @abstractmethod
    def preferences_options_are_visible(self) -> bool:
        raise NotImplementedError

    @abstractmethod
    def close_preferences(self) -> None:
        raise NotImplementedError

    @abstractmethod
    def preferences_are_closed(self) -> bool:
        raise NotImplementedError

    @abstractmethod
    def can_see_text(self, text: str) -> bool:
        raise NotImplementedError

    @abstractmethod
    def history_diff_contains(self, text: str) -> bool:
        raise NotImplementedError

    @abstractmethod
    def has_app_shell(self) -> bool:
        raise NotImplementedError

    @abstractmethod
    def toggle_toolbox_panel(self) -> None:
        raise NotImplementedError

    @abstractmethod
    def open_toolbox_panel(self) -> None:
        raise NotImplementedError

    @abstractmethod
    def toolbox_shell_is_visible(self) -> bool:
        raise NotImplementedError

    @abstractmethod
    def flowboard_has_card(self, title: str) -> bool:
        raise NotImplementedError

    @abstractmethod
    def create_child_card_from_flowboard(self, parent_title: str) -> None:
        raise NotImplementedError

    @abstractmethod
    def switch_toolbox_tool(self, tool_name: str) -> None:
        raise NotImplementedError

    @abstractmethod
    def add_url_resource(self, label: str, uri: str) -> None:
        raise NotImplementedError

    @abstractmethod
    def filter_resources(self, query: str) -> None:
        raise NotImplementedError

    @abstractmethod
    def delete_resource(self, label: str) -> None:
        raise NotImplementedError

    @abstractmethod
    def toggle_ai_panel(self) -> None:
        raise NotImplementedError

    @abstractmethod
    def search_panel_is_visible(self) -> bool:
        raise NotImplementedError

    @abstractmethod
    def ai_panel_is_visible(self) -> bool:
        raise NotImplementedError

    @abstractmethod
    def toolbox_panel_is_visible(self) -> bool:
        raise NotImplementedError

    @abstractmethod
    def toolbox_panel_is_hidden(self) -> bool:
        raise NotImplementedError
