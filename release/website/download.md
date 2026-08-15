---
layout: default
title: Download Holder
permalink: /download/
---

<section class="download-panel">
  <p class="eyebrow">Download</p>
  <h1>Get Holder</h1>
  <p class="lead">
    Holder Framework Edition includes the desktop app, local backend service
    and command-line tools.
  </p>
  <div class="button-row">
    <a class="button primary" href="https://github.com/HolderTeam/holder-release/releases/latest" data-download-primary>View downloads</a>
    <a class="button secondary" href="https://github.com/HolderTeam/holder-release/releases/latest">Latest release</a>
  </div>
  <p class="download-note">
    Detected platform: <span data-download-platform>checking...</span>.
    <span data-download-detail>Looking up the latest release.</span>
  </p>
</section>

<section class="section">
  <div class="section-head">
    <h2>Choose your platform.</h2>
    <p>
      The recommended button above tries to choose a matching release asset.
      These links stay useful if browser detection fails.
    </p>
  </div>

  <div class="platform-list">
    <article class="platform-card" data-platform-card="windows">
      <div>
        <h3>Windows</h3>
        <p>Installer for Windows 10 and Windows 11.</p>
      </div>
      <a class="button secondary" href="https://github.com/HolderTeam/holder-release/releases/latest">Download installer</a>
    </article>

    <article class="platform-card" data-platform-card="macos">
      <div>
        <h3>macOS</h3>
        <p>Signed and notarised DMG for Apple desktop systems.</p>
      </div>
      <a class="button secondary" href="https://github.com/HolderTeam/holder-release/releases/latest">Download DMG</a>
    </article>

    <article class="platform-card" data-platform-card="linux" id="linux">
      <div>
        <h3>Ubuntu</h3>
        <p>Install from the Holder PPA.</p>
      </div>
      <a class="button secondary" href="#ubuntu">Install with apt</a>
    </article>

    <article class="platform-card">
      <div>
        <h3>Other Linux and BSD</h3>
        <p>Packages are still being expanded. Source builds are available now.</p>
      </div>
      <a class="button secondary" href="#source">Build from source</a>
    </article>
  </div>
</section>

<section class="section" id="ubuntu">
  <div class="section-head">
    <h2>Ubuntu</h2>
    <p>Install Holder from the official Holder PPA.</p>
  </div>

  <pre><code>sudo add-apt-repository ppa:holderteam/holder
sudo apt update
sudo apt install holder</code></pre>
</section>

<section class="section">
  <div class="section-head">
    <h2>Edition status.</h2>
    <p>
      Framework Edition is the full local system. Bedrock Edition is the
      future sandboxed edition for app stores and mobile platforms.
    </p>
  </div>

  <div class="grid two">
    <article class="card">
      <span class="tag">Available first</span>
      <h3>Framework Edition</h3>
      <p>Desktop app, local backend, CLI tools and API for people who want the full Holder system.</p>
    </article>

    <article class="card">
      <span class="tag">Coming later</span>
      <h3>Bedrock Edition</h3>
      <p>A more sandboxed Holder for stores and mobile platforms.</p>
    </article>
  </div>
</section>

<section class="section" id="source">
  <div class="section-head">
    <h2>Build Holder yourself.</h2>
    <p>
      Holder is developed in public. Each repository includes platform-specific
      build notes as support lands.
    </p>
  </div>

  <ul class="repo-list">
    <li><a href="https://github.com/HolderTeam/holder-daemon">holder-daemon</a> - local backend and command-line tools</li>
    <li><a href="https://github.com/HolderTeam/holder-desktop">holder-desktop</a> - GTK desktop frontend</li>
    <li><a href="https://github.com/HolderTeam/holder-launcher">holder-launcher</a> - platform launcher</li>
    <li><a href="https://github.com/HolderTeam/holder-release">holder-release</a> - release assets</li>
  </ul>
</section>
