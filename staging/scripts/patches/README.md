# GTK AccessKit patches

`gtk-accesskit-presentation-labels.patch` fixes names for `GtkModelButton` entries
in GTK 4.24.1 with AccessKit C 0.18.0. GTK marks each internal label as
presentation and points the menu item's `labelled-by` relation at it. The bridge
maps that label to AccessKit's `GenericContainer` role. AccessKit's name
calculation reads a referenced node's text value only for the `Label` role;
for other roles it reads its direct label, which GTK had left unset. The native
macOS menu item therefore exists with an empty title, and the observer omits it.

Preserve plain text on these presentation labels as a direct AccessKit label.
They keep their generic role, so this does not expose duplicate standalone label
elements. An explicit accessible label still takes precedence. GTK's plain-text
getter strips mnemonic markers and markup. No Holder widget or action changes
are needed.

The shared runtime recipe applies the patch with `patch --batch --forward`;
a future incompatible source fails preparation for review. The patch checksum
is part of the runtime cache identity and retained source provenance. Remove
this patch when an upstream runtime passes the native regression without it.

Run from `apps/desktop/gtk` on macOS:

```bash
python3 scripts/accessible_runtime.py check-menus
```

This compiles a small GTK test and inspects its own AppKit accessibility objects,
without external accessibility permissions or Holder data. It checks names,
mnemonic stripping, disabled/check states, focus, bounds, native activation,
reopening, label changes and submenu activation. On 2026-10-15 the unmodified
runtime failed to find named entries; the patched runtime passed. Full VoiceOver
usability is a separate manual check.

This GTK/macOS build also emits frame-timing and surface-thaw diagnostics during
the automated popup lifecycle test; the native accessibility assertions pass.
