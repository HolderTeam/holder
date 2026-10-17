# Low-level Holder client

The regular `Holder` API covers common scripting tasks. For direct access to
the daemon's HTTP endpoints, use `holder.generated`. Supply both the daemon URL
and bearer token explicitly; this client does not discover local connection
details or read credentials from your machine.

```python
import os

from holder.generated import AuthenticatedClient
from holder.generated.api.default import get_health
from holder.generated.models import HealthResponse

with AuthenticatedClient(
    base_url="http://127.0.0.1:11499",
    token=os.environ["HOLDER_TOKEN"],
    raise_on_unexpected_status=True,
) as client:
    response = get_health.sync_detailed(client=client)
    if isinstance(response.parsed, HealthResponse):
        print(response.parsed.data.server_version)
    else:
        print(response.status_code, response.parsed)
```

`Client` supports public endpoints such as `get_ping`. Endpoint modules expose
`sync`, `sync_detailed`, `asyncio` and `asyncio_detailed`; use `async with` for
asynchronous clients. Detailed responses retain status, headers, raw bytes and
the parsed response. Documented HTTP errors return an `ErrorResponse` when the
contract specifies one; `raise_on_unexpected_status` applies to undocumented
statuses. Transport errors propagate from HTTPX.

This code is generated from the daemon's [OpenAPI contract](../../../api/openapi.yaml).
For regeneration and contribution instructions, see [development](development.md).
