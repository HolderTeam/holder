---
layout: default
title: Android Backup
permalink: /android/backups/
---

<section class="page-panel">
  <p class="eyebrow">Android</p>
  <h1>Backup</h1>
  <p class="lead">A small, automatic safety net for the "phone in the toilet" case — not a replacement for Project Sync.</p>
</section>

<section class="section">
  <div class="section-head">
    <h2>What this actually does</h2>
    <p>
      Holder keeps a small, rolling snapshot of your most recently updated cards, written into the app's own data
      folder. That file sits inside Android's <em>own</em> Auto Backup system — the OS-level feature, tied to your
      device's Google account backup, that already backs up most apps' data automatically. Holder doesn't upload
      anything itself, and this has nothing to do with the Google Drive connection under Settings → Storage — see
      the note at the bottom of this page.
    </p>
    <p>
      In practice: if Android's own device backup is doing its job (which is usually on by default and out of
      Holder's control), a recent copy of your cards exists somewhere even for a project you never connected to
      any Git host at all.
    </p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>What it deliberately isn't</h2>
    <p>
      This is a modest, last-resort safety net, not <a href="{{ '/android/sync/' | relative_url }}">Project Sync</a>.
      The snapshot is size-capped to comfortably fit Android's own Auto Backup quota, so it only ever holds your
      <em>most recently updated</em> cards — if you have more cards than fit, the oldest ones simply aren't in it.
      There's no version history either: it's a snapshot of how your cards look right now, not a log of how they
      got there.
    </p>
    <p>
      If you want real history, and every card regardless of how long ago you touched it, that's what
      <a href="{{ '/android/sync/' | relative_url }}">Project Sync</a> is for. Backup is what's still there if you
      never got around to setting that up, or a project was deliberately kept device-only.
    </p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Restoring from a backup</h2>
    <p>
      Settings → Backup has a "Load" button, and reinstalling Holder after losing a device offers the same thing
      automatically if it finds a snapshot. Either way, restoring reads whatever snapshot is actually on the
      device and creates a <strong>brand-new project</strong> for each project the snapshot contains — fresh
      identity, no attempt to merge into or overwrite anything already there. This is deliberate: a restored
      project has no real Git ancestry to speak of, so giving it a fresh identity avoids ever mixing it up with an
      existing project that might later get connected to the same remote.
    </p>
    <p>
      One consequence worth knowing: restoring the same backup twice creates duplicate projects, not an update.
      There's no matching or deduplication logic — this is a recovery aid for "no project at all right now," not
      an ongoing sync mechanism, so only restore a given backup once.
    </p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Not the same thing as Storage</h2>
    <p>
      Settings → Storage lets you connect Google Drive or S3-compatible storage — but that's for attachments
      (photos, files added to cards), and has no connection to this page at all. Connecting Google Drive under
      Storage does not back up your cards, and disconnecting it doesn't affect this snapshot either. If you want
      the fullest, most reliable copy of a project, <a href="{{ '/android/sync/' | relative_url }}">Project Sync</a>
      is the one designed for that job.
    </p>
  </div>
</section>
