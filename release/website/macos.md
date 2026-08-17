---
layout: default
title: Holder for macOS
permalink: /macos/
---

<section class="page-panel">
  <p class="eyebrow">macOS</p>
  <h1>Holder for macOS</h1>
  <p class="lead">Download the Holder disk image, drag Holder into Applications, then open it like a normal Mac app.</p>
  <div class="actions">
    <a class="button primary" data-download-asset="macos" href="https://github.com/HolderTeam/holder-release/releases/download/holder-v0.1.7/Holder-macos-0.1.7.dmg">Download for macOS</a>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Install</h2>
    <p>Download the <a href="https://github.com/HolderTeam/holder-release/releases/download/holder-v0.1.7/Holder-macos-0.1.7.dmg">Holder disk image</a> (<code>.dmg</code>) and open it.</p>
    <p>A small window will appear containing <strong>Holder</strong> and an <strong>Applications</strong> folder. Drag the Holder icon onto the Applications folder. This copies <code>Holder.app</code> into your Applications folder.</p>
    <p>You can then close the window and eject the Holder disk image.</p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Getting Started</h2>
    <p>Open <strong>Finder</strong>, select <strong>Applications</strong>, and double-click <strong>Holder</strong>.</p>
    <p>The first time you run Holder, macOS will tell you that the application was downloaded from the internet and ask whether you're sure you want to open it. Click <strong>Open</strong>.</p>
    <p>That's it. On subsequent launches you can open Holder normally from Applications, Spotlight or the Dock.</p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Current macOS Status</h2>
    <p>The macOS build is packaged as a standard <code>.app</code> inside a disk image. It includes the desktop app, launcher, backend daemon and command-line tool.</p>
    <p>The release build is signed and notarised for direct download outside the Mac App Store.</p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Building From Source</h2>
    <p>macOS source builds currently use Homebrew for dependencies. The packaged app is assembled from three repositories.</p>
    <ul class="steps">
      <li>Install Xcode Command Line Tools and Homebrew.</li>
      <li>Clone the backend: <code>git clone https://github.com/HolderTeam/holder-daemon.git</code>.</li>
      <li>In <code>holder-daemon</code>, install the Homebrew dependencies listed in the README, then run <code>./make.sh</code>.</li>
      <li>Clone the desktop app: <code>git clone https://github.com/HolderTeam/holder-desktop.git</code>.</li>
      <li>In <code>holder-desktop</code>, install the Homebrew dependencies listed in the README, then run <code>meson setup build-macos --prefix=/</code>, <code>meson compile -C build-macos</code>, and <code>meson test -C build-macos --print-errorlogs</code>.</li>
      <li>Run <code>./build-macos/holder-desktop</code> only after the backend is already running.</li>
    </ul>
    <p>The release app also includes the small launcher from <a href="https://github.com/HolderTeam/holder-launcher">holder-launcher</a>. Most developers do not need to build it unless they are working on the macOS app bundle or packaging.</p>
  </div>
</section>
