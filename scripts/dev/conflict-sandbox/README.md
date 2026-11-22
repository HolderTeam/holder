# Conflict sandbox

Builds a throwaway Holder full of Git conflict copies, so you can open Desktop and try
**Git Sync → Compare versions** without setting up two machines.

```sh
python3 scripts/dev/conflict-sandbox/sandbox.py setup     # build every scenario
python3 scripts/dev/conflict-sandbox/sandbox.py desktop   # open Desktop on the sandbox
python3 scripts/dev/conflict-sandbox/sandbox.py reset     # stop it and delete everything
```

On Windows use `py` instead of `python3`.

## What it does

Everything lives in one sandbox folder (default `<temp>/holder-conflict-sandbox`, change it
with `--root`):

- Holder's data, config, cache and state point into the sandbox through `XDG_*` variables, and
  project keys go to files there (`HOLDER_TEST_KEYSTORE_DIR`) instead of your keyring.
- Its own `holderd` runs on port 11599 (`--port`), started directly rather than through a system
  service, and stops itself after four idle hours. Your real daemon on 11499 is not touched.
- For each scenario, one plain project, `Conflict: <scenario>`, with a local bare remote in
  `remotes/` and an "other machine" clone in `other/`.

For each scenario the script pushes the starting cards, then makes a change through Holder
(committed, not pushed) and a conflicting change by editing the card file in the other clone
with plain Git and pushing it. Holder's pull then finds the card changed on both sides: the
other machine's version stays as the original, and this machine's version becomes
`<title> (conflicted copy)`.

## Scenarios

These end in a conflict copy to compare:

| Name | What to look at |
| --- | --- |
| `basic` | The same line edited on both sides; the other machine also renamed the card. |
| `children` | A parent and child both conflict, so the copy has a child of its own. Cleanup shows where it would move. |
| `original-under-copy` | The original has been moved under its copy, so **Move copy to Trash** is blocked. |
| `copy-extras` | Tags, a link and a milestone only on the copy, and another card linking to the copy. |
| `large-diff` | 1,200 different lines on each side: too large to highlight. |
| `crlf` | CRLF line endings, to check that highlights land on the right lines. Both sides edited line two; the other also edited line four. |
| `both-move` | Both machines moved the card to different places. |
| `neighbouring-lines` | This machine edited a line; the other added a line just below it. Edits that touch neighbouring lines are not merged. |

These end without a copy: the two machines changed different parts of the card, or (for
`trash-vs-edit` and `edit-vs-trash`) Holder keeps the edited version:

| Name | What happened |
| --- | --- |
| `move-vs-edit` | This machine only moved the card; the other edited its text. |
| `identical-edit` | Both machines made the same edit. |
| `title-vs-body` | This machine renamed the card; the other added a paragraph. |
| `different-lines` | Both machines edited the text, on lines apart from each other: both edits are kept. |
| `link-vs-edit` | This machine added a link; the other edited the text. |
| `trash-vs-edit` | This machine moved the card to Trash while the other edited it. The edited card stays live and out of Trash, so it can be trashed again. |
| `edit-vs-trash` | This machine edited the card while the other moved it to Trash. The card comes back out of Trash with this machine's edit. |

Setup flags any scenario whose outcome differs from this and exits nonzero, so it doubles as a
quick check of the merge rules against a daemon build.

`setup basic crlf` builds only those. `list` prints them. Running `setup` again for a scenario
that exists asks you to `reset` first.

## Seeing the toasts

Setup pulls before Desktop opens, and Desktop only announces pulls it sees while running. To see
the pull toast, open Desktop on the sandbox, select a scenario's project, then run:

```sh
python3 scripts/dev/conflict-sandbox/sandbox.py toast            # a fresh conflict in "basic"
python3 scripts/dev/conflict-sandbox/sandbox.py toast --merge    # edits lines apart: a merge
python3 scripts/dev/conflict-sandbox/sandbox.py toast copy-extras
```

Each run edits the card with the most lines on both sides, the other machine pushes, and the
sandbox daemon pulls, as a background pull would. Desktop then shows "Pulled changes from another
device…" with Compare. Resolving the conflict in the dialog shows the Undo toast. Run it as often
as you like; the scenario has to exist (`setup <scenario>`) first.

## Options and binaries

By default the script uses `daemon/build/holderctl` and `holderd` (also `Debug/`, `Release/` and
`RelWithDebInfo/` beneath it) and `apps/desktop/gtk/build/holder-desktop`. Override them with
`--holderctl`, `--holderd` and `--desktop`, or `HOLDER_SANDBOX_HOLDERCTL`,
`HOLDER_SANDBOX_HOLDERD` and `HOLDER_SANDBOX_DESKTOP`. Global options go before the command:
`sandbox.py --root /tmp/sbx setup`.

It needs `git` on `PATH` and Python 3.11+ with `httpx`, `attrs` and `typing-extensions`. It
uses the Holder Python library from this checkout if it isn't installed.

To use `holderctl` or your own scripts against the sandbox, load its environment:

```sh
eval "$(python3 scripts/dev/conflict-sandbox/sandbox.py env)"      # bash/zsh
python3 scripts/dev/conflict-sandbox/sandbox.py env | Invoke-Expression   # PowerShell
```

## Troubleshooting

- **"Settings schema 'team.holder.Holder' does not contain a key named …"**: an older Holder
  schema is installed on the machine. `desktop` puts the build's own schema first, so update this
  script. Desktop run that way keeps its preferences in `<root>/config`, apart from your own, and
  `reset` deletes them.

- **"The other machine's push was rejected"**: the sandbox daemon's background sync pushed
  first. Run `reset` and `setup` again.
- **The daemon will not start**: check that port 11599 is free, or pass another `--port`. The
  start log is in `<root>/cache/holder/`.
- `reset` deletes only folders this script created (marked with `.holder-conflict-sandbox`).
