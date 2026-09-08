---
layout: default
title: Android Storage
permalink: /android/storage/
---

<section class="page-panel">
  <p class="eyebrow">Android</p>
  <h1>Storage</h1>
  <p class="lead">Where photos and files attached to your cards actually live — separate from the cards themselves.</p>
</section>

<section class="section">
  <div class="section-head">
    <h2>What this is for</h2>
    <p>
      A card's own text — its title, body, tags, dates — is never stored here. That's what
      <a href="{{ '/android/sync/' | relative_url }}">Project Sync</a> and <a href="{{ '/android/backups/' | relative_url }}">Backup</a>
      are for. Storage is specifically for the attachments you add to a card: photos, files, anything that isn't
      plain text. Settings → Storage lets you connect one of two kinds of place to keep those.
    </p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Google Drive</h2>
    <p>
      Connect your own Google Drive account through an ordinary Google sign-in. Simple, and enough for most
      people's needs.
    </p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>S3-compatible storage</h2>
    <p>
      For anyone already running their own storage — a real AWS S3 bucket, or any S3-compatible service — Holder
      can connect to it directly instead: an endpoint, a region, a bucket name, and a set of access keys. Unlike
      Google Drive's sign-in, this is entered by hand, and the credentials themselves stay on this device rather
      than passing through anything else. The endpoint and bucket name travel with the project through Git, so
      other devices connected to the same project know where to look, but each device still needs its own
      credentials entered to actually use it.
    </p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Not the same thing as Sync or Backup</h2>
    <p>
      It's easy to assume connecting Google Drive here means your cards are now backed up to Drive. They aren't —
      this page has no effect on your cards' own text at all, only on attachments. If you're looking for how card
      content itself gets kept safe or shared across devices, see <a href="{{ '/android/sync/' | relative_url }}">Project Sync</a>
      and <a href="{{ '/android/backups/' | relative_url }}">Backup</a> instead.
    </p>
  </div>
</section>
