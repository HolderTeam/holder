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
    <figure class="page-image">
      <img src="{{ '/assets/img/platforms/macos/mac1-dmg-drag.png' | relative_url }}" alt="The Holder macOS disk image showing Holder.app being dragged to Applications">
    </figure>
    <p>You can then close the window and eject the Holder disk image.</p>
    <p>Open <strong>Finder</strong>, select <strong>Applications</strong>, and double-click <strong>Holder</strong>.</p>
    <p>The first time you run Holder, macOS will tell you that the application was downloaded from the internet and ask whether you're sure you want to open it. Click <strong>Open</strong>.</p>
    <figure class="page-image">
      <img src="{{ '/assets/img/platforms/macos/mac2-app_from_internet.png' | relative_url }}" alt="macOS asking whether to open Holder because it was downloaded from the internet">
    </figure>
    <p>That's it. On subsequent launches you can open Holder normally from Applications, Spotlight or the Dock.</p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Getting Started</h2>

    <p>Holder opens with your projects and cards. By default you get a project called Home. Press the plus button to create a card, or the folder-plus button to create a project.</p>
    <figure class="page-image">
      <img src="{{ '/assets/img/platforms/macos/mac3-first-open.png' | relative_url }}" alt="Holder running on macOS with the default Home project open">
    </figure>
    <p>Press the beaker icon to open the toolbox, press the keyboard icon to open the AI panel.</p>
    <figure class="page-image">
      <img src="{{ '/assets/img/platforms/macos/mac4_tool_and_ai_trays_open.png' | relative_url }}" alt="Holder running on macOS with the toolbox and AI panel visible">
    </figure>
    <p>To change appearance, open the hamburger menu and choose <strong>Preferences</strong>.</p>
    <figure class="page-image">
      <img src="{{ '/assets/img/platforms/macos/mac5_hamburger_menu.png' | relative_url }}" alt="Holder on macOS with the hamburger menu open and Preferences visible">
    </figure>
    <p>Then you can change the window style and the card editor theme.</p>
    <figure class="page-image">
      <img src="{{ '/assets/img/platforms/macos/mac6_preferences.png' | relative_url }}" alt="Holder Preferences on macOS showing appearance settings">
    </figure>
    <p>Holder Framework Edition gives you several ways to work with the same local knowledge base: the desktop app, the command line and the HTTP API.</p>
    <p>The local HTTP API is documented with Swagger UI.</p>
    <figure class="page-image">
      <img src="{{ '/assets/img/platforms/macos/mac8_swagger_docs.png' | relative_url }}" alt="Holder API documentation open in a browser on macOS">
    </figure>
    <p>The command-line tool is bundled with the app. For now, you can run it from Terminal with <code>/Applications/Holder.app/Contents/Resources/bin/holderctl</code>.</p>
    <figure class="page-image">
      <img src="{{ '/assets/img/platforms/macos/mac7_holderctl.png' | relative_url }}" alt="holderctl running in a Terminal on macOS">
    </figure>

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
