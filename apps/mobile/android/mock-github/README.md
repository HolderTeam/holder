# Mock GitHub

A local stand-in for GitHub and Holder's sign-in relay, for testing the Android app's GitHub
connection on a phone or emulator without the real services. Its main use is making things go
wrong on purpose: garbled or lost replies, server errors, slow replies, expired or revoked
tokens. Real GitHub cannot be made to do that on demand.

`mock_github.py` is one Python file using only the standard library. It listens on
`127.0.0.1` only.

## Running it

```sh
cd apps/mobile/android
python3 mock-github/mock_github.py serve          # port 8765; Ctrl-C stops it
scripts/deploy-all.sh --mock                      # in another terminal
```

`deploy-all.sh --mock` builds the debug app with `-Pholder.githubMock=http://127.0.0.1:8765`,
installs it on every connected phone and emulator, and runs `adb reverse tcp:8765 tcp:8765` on
each, so the device's `127.0.0.1:8765` reaches this machine. Run `scripts/deploy-all.sh`
without `--mock` to go back to the real GitHub.

Then open <http://127.0.0.1:8765/> on this machine: the control page. It lists every request
the app makes and has buttons for the faults below. In the app, Settings → Sync → GitHub →
Connect opens the mock's sign-in page; tap **Authorize**.

A sign-in stored by a real-GitHub build does not work against the mock, and the other way
round: after switching, Disconnect and Connect again.

The same controls from a terminal:

```sh
python3 mock-github/mock_github.py fault refresh garbled   # the next refresh misbehaves
python3 mock-github/mock_github.py fault api delay --seconds 30
python3 mock-github/mock_github.py expire                  # access tokens expire now
python3 mock-github/mock_github.py revoke                  # as if revoked on github.com
python3 mock-github/mock_github.py set access_ttl=90 installed=false
python3 mock-github/mock_github.py status                  # everything, as JSON
```

The app keeps its access token only in memory, so every cold start (force-stop, then open)
begins with a refresh. That makes refresh faults quick to exercise: queue one, then restart
the app.

## Faults

Each fault applies to the next request to that endpoint only, in the order queued. Every
endpoint also takes `delay` (wait, 20 seconds by default, then reply normally), which is the
way to tap Disconnect or Connect while a refresh is in flight.

For `exchange` and `refresh`, the faults come in two kinds, matching what the relay promises:

| Fault | Credential | Reply |
| --- | --- | --- |
| `unexpected` | not spent | 500 `{"error":"unexpected"}` |
| `rate_limited` | not spent | 429 `{"error":"rate_limited","retry_after_seconds":30}` |
| `invalid_request` | not spent | 400 `{"error":"invalid_request"}` |
| `reconnect` | rejected | 400 `{"error":"authorization_required","reason":"reconnect"}` |
| `verify_email` | not spent | 400 `{"error":"authorization_required","reason":"verify_email"}` |
| `garbled` | **spent** | 200 with the body `{broken` |
| `partial` | **spent** | 200 with the tokens but no `refresh_cap` |
| `outcome_unknown` | **spent** | 503 `{"error":"outcome_unknown"}` |
| `html` | **spent** | 502 with an HTML error page |
| `oversize` | **spent** | 200 with a 20 KiB body |
| `drop` | **spent** | the connection closes with no reply |

"Spent" means the mock used the code, or rotated the refresh token, before losing the reply.
That is the case the app must treat as unsafe: it must not send that refresh token again. If it
does, the mock rejects it, and the control page and `status` count it as a **replay**. A
replay count above zero is a bug in the app.

For `api`: `unauthorized` (401), `server_error` (500), `rate_limited` (429), `drop`.

Other switches: **Expire access tokens now** (as if eight hours passed), **Revoke the app**
(every access and refresh token stops working, as when the user revokes Holder on github.com),
the access token lifetime (the app refreshes 60 seconds early, so a lifetime under about 90
seconds means a refresh before every request), whether the app is installed on the account
(off gives the app's "Finish setup" state), and **Skip the Authorize page**.

Skipping the Authorize page only works in browsers with Auth Tab support (current Chrome).
With Custom Tabs, Chrome hands the callback link to the app only after a tap, so the tab stays
on the callback page instead. Real GitHub skips the button for an app already authorized, so
this mimics that case too.

## What it does not cover

* Git itself. Repositories it creates have the SSH URL
  `git@mock-github.invalid:<owner>/<name>.git`, so a push or pull fails. Use it for the sign-in
  and provisioning steps, not for syncing card data.
* The "Finish setup" and "Manage repository access" links still open the real github.com.
* Browser and App Link behaviour. The sign-in page sends the browser to the app's real
  callback, `https://auth-dev.holder.ws/android/oauth-callback`, so the device must have the
  debug app's App Link for that host verified, as it does for the real sign-in.

## The relay protocol, as the app uses it

Two endpoints, both `POST` with `Content-Type: application/json` (an optional
`charset=utf-8` parameter is allowed; anything else gets 415 with no body). Request bodies are
at most 4096 bytes and must have exactly the listed fields, each a non-empty string, or the
reply is 400 `invalid_request`.

* `/v1/github/exchange`: `{"code": ..., "code_verifier": ...}`. The code is the one GitHub's
  authorization page returned to the callback. The verifier is the PKCE (S256) verifier whose
  challenge went to the authorization page.
* `/v1/github/refresh`: `{"refresh_token": ..., "refresh_cap": ...}`. Both come from the last
  successful reply. The cap is an opaque value the relay issues alongside each refresh token;
  a token sent with the wrong cap is rejected with `reconnect` without being spent.

A successful reply is 200 with exactly these fields:

```json
{
  "access_token": "ghu_...",
  "expires_in": 28800,
  "refresh_token": "ghr_...",
  "refresh_cap": "...",
  "refresh_token_expires_in": 15897600
}
```

The tokens are strings, the lifetimes positive integers in seconds. Each successful refresh
replaces the refresh token and cap; the old refresh token never works again.

Every failure is a flat JSON object with an `error` field, at a fixed HTTP status:

| Status | Body | Meaning for the credential |
| --- | --- | --- |
| 400 | `{"error":"invalid_request"}` | request malformed; nothing spent |
| 400 | `{"error":"authorization_required","reason":"reconnect"}` | code or refresh token rejected; sign in again |
| 400 | `{"error":"authorization_required","reason":"verify_email"}` | the GitHub account must verify its email |
| 429 | `{"error":"rate_limited","retry_after_seconds":N}` | nothing spent; try again after N seconds (also sent as `Retry-After`) |
| 500 | `{"error":"unexpected"}` | the relay failed; nothing spent, safe to retry |
| 503 | `{"error":"outcome_unknown"}` | GitHub may have spent the credential; do not reuse it |

The app accepts only these exact shapes. Any other status, any extra field, or a body that is
not this JSON (an HTML error page, a truncated body, more than 16 KiB) means the app cannot
know whether the credential was spent, so it treats it like `outcome_unknown`. A lost
connection after the request body started being sent is treated the same way.

## Tests

```sh
cd apps/mobile/android/mock-github
python3 -m unittest test_mock_github
```
