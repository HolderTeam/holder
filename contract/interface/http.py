"""The daemon's HTTP routes for the contract's operations. Core ignores this file."""

from holder_contract import http

from .cards import create_card, get_card, list_cards

http(get_card, "GET", "/cards/{card_id}")
http(list_cards, "GET", "/projects/{project_id}/cards",
     query=["parent", "tag", "order", "limit", "cursor", "include_content", "include_child_count"])
http(create_card, "POST", "/cards")
