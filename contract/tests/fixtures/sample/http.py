"""The sample contract's HTTP layer."""

from holder_contract import http, http_error

from .cards import Conflict, NotFound, get_asset_content, get_card, list_cards, run_ai, update_message

http_error(NotFound, 404)
http_error(Conflict, 409)

http(get_card, "GET", "/cards/{card_id}")
http(list_cards, "GET", "/projects/{project_id}/cards", query=["parent", "limit", "cursor"])
http(update_message, "PATCH", "/ai/messages/{message_id}")
http(get_asset_content, "GET", "/assets/{asset_id}/content")
http(run_ai, "POST", "/ai/runs", status=201)
