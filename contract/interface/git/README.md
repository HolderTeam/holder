# Local file edits during pull

Holder leaves outside edits in the working tree. Pull never stages them or creates a commit on their behalf.

- A fast-forward may proceed when the incoming changes do not overwrite an unstaged file. This applies to tracked live and Trash cards as well as other tracked files. An overlapping change fails with the blocking path when Git can identify it. Review and commit valid work, or back it up and restore tracked files or move untracked files aside before retrying.
- A staged or unresolved index, an unstaged deletion, or a file type change stops pull. This keeps an unrelated staged change out of any merge or repair commit. Pull never removes unrelated untracked or ignored files. Diverged branches require a working tree clear of tracked changes and non-ignored untracked files before Holder can merge them. Ignored files that would be overwritten also block the merge.
- After Git succeeds, the SQLite projection is rebuilt from the working tree. Card IDs must be valid and match the file's live or Trash location. The checkout's project record supplies the project ID. Legacy Markdown without front matter can still derive its card ID from the canonical filename. Encrypted card files with an available key must decrypt and pass the same validation. An invalid file leaves its bytes in place and rolls back the SQLite rebuild. Git may already have advanced to an independent incoming commit. Fix the file and retry to reconcile.
- Hierarchy repair waits for a clean working tree because it can rewrite and commit card files. A pull blocked by local work or a failed rebuild is reported through the existing pull error and skips the paired push, including in scheduled sync.

## Conflict copies

A divergent pull can preserve a committed local card version under a new card ID. Holder records the original ID through a `conflict_copy_of` link in the preserved card's front matter. The index rebuilds this link from the card file. Renames, Trash, and permanent deletion of the original leave the reference intact while the copy exists. Holder manages these links through sync.

The C API and HTTP sync-status operations expose live conflict copies with their current titles and original IDs. An original may be in Trash or absent. Pairs survive subsequent pulls and are visible after a desktop refresh, including when the daemon created the copies in the background. Copies made by older versions may lack an association. Holder never infers it from a title.

Pull results count only new cards written to preserve a local version. Unrelated file conflicts and unreadable local versions do not increase that count. If writing a copy fails, the pull fails and names the card path and the pre-merge commit. Any copies already preserved remain discoverable.
