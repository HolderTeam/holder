# GTK accessibility patches

`gtk-menu-label-role.patch` fixes names for `GtkModelButton` menu entries by
constructing their internal text with the normal `LABEL` role instead of
`PRESENTATION`. The existing `labelled-by` relation then points to a named
accessible label, matching ordinary `GtkButton` behavior. This fixes GTK's own
name calculation as well as the native AccessKit output.

This replaces Holder's earlier AccessKit bridge workaround, which preserved
plain text on a presentation label only in the backend. The normal-role change
handles mnemonic stripping, markup, explicit name overrides and text updates
through GTK's existing label behavior; no Holder widgets or actions change.

A comparison against upstream GTK 4.25.0 on 2026-10-15 found that all three
candidates fixed the basic native menu regression. The direct-label approach
from GTK MR !7151, rebased locally, overwrote explicit names on text changes and
exposed markup tags. The bridge workaround and normal-role approach preserved
those cases, but only the normal-role approach also fixed GTK's computed name.
The focused GTK button and name tests passed for all candidates.

The shared runtime recipe applies this one-line patch with
`patch --batch --forward`; a future incompatible source fails preparation for
review. The patch name and checksum are part of the runtime cache identity and
retained source provenance, so replacing the bridge patch selects a fresh cache.
Remove the patch when an upstream runtime passes the regression without it.

Run from `apps/desktop/gtk` on macOS:

```bash
python3 scripts/accessible_runtime.py check-menus
```

This compiles a small GTK test and inspects its own AppKit accessibility objects,
without external accessibility permissions or Holder data. It checks the normal
label role, names, mnemonic stripping, markup, disabled/check states, focus,
bounds, native activation, explicit names across text updates and reset,
reopening, label changes and submenu activation. The expanded regression rejects
the old bridge workaround and passes the normal-role prototype.

The normal-role change exposes a static-text child beneath each native menu
item, as ordinary buttons already do. Check with VoiceOver that this does not
cause duplicate announcements or extra navigation stops. End-to-end VoiceOver
usability and Linux AT-SPI/Orca behavior remain unverified.

This GTK/macOS build also emits frame-timing and surface-thaw diagnostics during
automated popup lifecycle tests; the native accessibility assertions pass.
