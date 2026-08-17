---
layout: default
title: Holder
---

<section class="hero">
  <div class="hero-copy">
    <p class="eyebrow">Your knowledge belongs to you.</p>
    <h1>Holder</h1>
    <p class="lead">
      A local-first workspace for cards, projects, resources and AI threads.
      Download Holder for your computer and keep your knowledge on your own machine.
    </p>
    <div class="hero-actions">
      <a class="button primary" href="https://github.com/HolderTeam/holder-release/releases/tag/holder-v0.1.7" data-download-primary>Download Holder</a>
      <a class="button secondary" href="#download">Choose platform</a>
    </div>
    <p class="download-note">
      Detected platform: <span data-download-platform>checking...</span>.
      <span data-download-detail>Looking up the latest release.</span>
    </p>
  </div>

  <div class="hero-visual">
    <img src="{{ '/assets/img/holder.jpg' | relative_url }}" alt="A Holder card box containing index cards">
  </div>
</section>

<section class="section" id="download">
  <div class="section-head">
    <h2>Download Holder</h2>
    <p>Install the current Framework Edition for Linux, Windows or macOS.</p>
  </div>

  <div class="platform-list">
    <article class="platform-card" data-platform-card="linux">
      <div>
        <h3>Linux</h3>
        <p>Install from the official Holder PPA. <a class="muted-link" href="{{ '/linux/' | relative_url }}">Linux notes</a></p>
      </div>
      <a class="button secondary" href="{{ '/linux/' | relative_url }}">Linux install</a>
    </article>

    <article class="platform-card" data-platform-card="windows">
      <div>
        <h3>Windows</h3>
        <p>Installer for Windows 10 and Windows 11. <a class="muted-link" href="{{ '/windows/' | relative_url }}">Windows notes</a></p>
      </div>
      <a class="button secondary" href="https://github.com/HolderTeam/holder-release/releases/download/holder-v0.1.7/Holder-windows-0.1.7-dev-Setup.exe" data-download-asset="windows">Download installer</a>
    </article>

    <article class="platform-card" data-platform-card="macos">
      <div>
        <h3>macOS</h3>
        <p>Signed and notarised DMG. <a class="muted-link" href="{{ '/macos/' | relative_url }}">macOS notes</a></p>
      </div>
      <a class="button secondary" href="https://github.com/HolderTeam/holder-release/releases/download/holder-v0.1.7/Holder-macos-0.1.7.dmg" data-download-asset="macos">Download DMG</a>
    </article>

  </div>
</section>
