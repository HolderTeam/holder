"""The daemon's HTTP routes for the contract's operations. Core ignores this file."""

from holder_contract import http, http_error

from .cards import create_card, get_card, list_cards
from .errors import Conflict, DataCorrupted, KeyMissing, KeyringUnavailable, NotFound

http_error(NotFound, 404)
http_error(Conflict, 409)
# The project's state rules the request out until its key is restored.
http_error(KeyMissing, 409)
http_error(KeyringUnavailable, 503)
# Not something the caller can fix; the code names the condition.
http_error(DataCorrupted, 500)

http(get_card, "GET", "/cards/{card_id}")
http(list_cards, "GET", "/projects/{project_id}/cards",
     query=["parent", "tag", "order", "limit", "cursor", "include_content", "include_child_count"])
http(create_card, "POST", "/cards", status=201)
