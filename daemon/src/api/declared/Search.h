#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"

#include <optional>
#include <string>

namespace holder::api {

// Declared for the contract; the existing route handlers still write these responses.

RIG_ROUTE(GET, "/search/cards")
RIG_SUMMARY("Search cards")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
SearchCardsResponse get_search_cards(
    std::string project_id RIG_QUERY,
    std::string q RIG_QUERY,
    std::optional<int> limit RIG_QUERY,
    std::optional<int> offset RIG_QUERY);

RIG_ROUTE(GET, "/search/ai")
RIG_SUMMARY("Search AI messages")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
SearchMessagesResponse get_search_ai(
    std::string project_id RIG_QUERY,
    std::string q RIG_QUERY,
    std::optional<int> limit RIG_QUERY,
    std::optional<int> offset RIG_QUERY);

} // namespace holder::api
