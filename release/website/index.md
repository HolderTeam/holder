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
    <img src="{{ '/assets/img/holder_screenshot.png' | relative_url }}" alt="A Screenshot of Holder">
  </div>
</section>

<section class="section" id="download">
  <div class="section-head">
    <h2>Download Holder</h2>
    <p>Choose the current Holder build for Ubuntu, Linux, Windows or macOS. Build versions vary by platform while Holder approaches 0.2.0.</p>
  </div>

  <div class="platform-list">
    <article class="platform-card" data-platform-card="ubuntu">
      <div>
        <div class="platform-heading">
          <h3>Ubuntu</h3>
          <span class="tag">0.1.7 stable</span>
        </div>
        <p>Install from the official HolderTeam PPA. <a class="muted-link" href="{{ '/ubuntu/' | relative_url }}">Ubuntu notes</a></p>
      </div>
      <a class="button secondary" href="{{ '/ubuntu/' | relative_url }}" data-download-asset="ubuntu">Install on Ubuntu</a>
    </article>

    <article class="platform-card" data-platform-card="linux">
      <div>
        <div class="platform-heading">
          <h3>Linux</h3>
          <span class="tag preview">0.2.0-dev preview</span>
        </div>
        <p>Signed AppImage preview for x86_64 Linux. <a class="muted-link" href="{{ '/linux/' | relative_url }}">Linux notes</a></p>
      </div>
      <a class="button secondary" href="{{ site.data.downloads.platforms.linux.url }}" data-download-asset="linux">Download AppImage</a>
    </article>

    <article class="platform-card" data-platform-card="macos">
      <div>
        <div class="platform-heading">
          <h3>macOS</h3>
          <span class="tag">0.1.7 stable</span>
        </div>
        <p>Signed and notarised disk image. <a class="muted-link" href="{{ '/macos/' | relative_url }}">macOS notes</a></p>
      </div>
      <a class="button secondary" href="{{ site.data.downloads.platforms.macos.url }}" data-download-asset="macos">Download DMG</a>
    </article>

    <article class="platform-card" data-platform-card="windows">
      <div>
        <div class="platform-heading">
          <h3>Windows</h3>
          <span class="tag preview">0.1.7 development</span>
        </div>
        <p>Self-signed installer for Windows 10 and Windows 11. Please really do read the <a class="muted-link" href="{{ '/windows/' | relative_url }}">Windows notes</a></p>
      </div>
      <a class="button secondary" href="{{ site.data.downloads.platforms.windows.url }}" data-download-asset="windows">Download installer</a>
    </article>

  </div>
</section>
