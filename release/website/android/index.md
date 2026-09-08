---
layout: default
title: Holder for Android
permalink: /android/
---

<section class="page-panel">
  <p class="eyebrow">Android</p>
  <h1>Holder for Android</h1>
  <p class="lead">In active development, not on the Play Store yet.</p>
</section>

<section class="section">
  <div class="section-head">
    <h2>Where things stand</h2>
    <p>
      The Android app is real and being actively worked on — cards, projects, encryption, and
      <a href="{{ '/android/sync/' | relative_url }}">Project Sync</a> already work — but it isn't published anywhere
      yet. There's no Play Store listing and no signed public build to download. It's currently built and tested
      from source, the same as the rest of Holder in this phase.
    </p>
    <p>
      If you'd like to try it before then, or help get it there, the source is open — see
      <a href="https://github.com/HolderTeam/holder-android">holder-android on GitHub</a>.
    </p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Already running a build?</h2>
    <p>These cover the three settings people mix up most:</p>
  </div>
  <div class="grid three">
    <article class="card">
      <h2><a href="{{ '/android/sync/' | relative_url }}">Project Sync</a></h2>
      <p>Full project history, kept up to date across your devices — the GitHub paved road, or your own Git server.</p>
    </article>
    <article class="card">
      <h2><a href="{{ '/android/backups/' | relative_url }}">Backup</a></h2>
      <p>A small, automatic safety net for your most recent cards, riding on Android's own device backup.</p>
    </article>
    <article class="card">
      <h2><a href="{{ '/android/storage/' | relative_url }}">Storage</a></h2>
      <p>Where photos and files attached to your cards live — Google Drive or your own S3-compatible bucket.</p>
    </article>
  </div>
</section>
