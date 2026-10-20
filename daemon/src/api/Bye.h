#pragma once

#include "api/RigAnnotations.h"

namespace holder::api {

struct ByeResponse {
  bool ok RIG_BOOL_VALUE(true);
};

RIG_ROUTE(POST, "/bye")
RIG_SUMMARY("Say that this client is leaving")
RIG_DESCRIPTION(
    "A hint, never a command. A daemon started with `--idle-exit` normally waits out its whole "
    "idle period after the last client has gone; after this request it waits only a few seconds "
    "(3), and only while nothing else happens. Running work, an open event stream, a new "
    "connection or any other request keeps it running, so another client is never cut off by "
    "one client leaving. A daemon without `--idle-exit`, such as the systemd service, ignores "
    "it. There is no request body, and nothing is returned beyond acknowledgement. Clients call "
    "it best-effort as they close; a client that crashes simply never does, and the daemon then "
    "falls back to its idle period."
)
RIG_RESPONSE_DESCRIPTION("Acknowledged")
ByeResponse say_goodbye();

} // namespace holder::api
