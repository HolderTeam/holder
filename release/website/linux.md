---
layout: default
title: Holder for Linux
permalink: /linux/
---

<section class="page-panel">
  <p class="eyebrow">Linux</p>
  <h1>Holder for Linux</h1>
  <p class="lead">
    Try the signed Holder 0.2.0-dev AppImage preview on x86_64 Linux. It runs without installing Holder system-wide.
  </p>
  <div class="actions">
    <a class="button primary" data-download-asset="linux" href="{{ site.data.downloads.platforms.linux.url }}">Download the AppImage</a>
    <a class="button secondary" href="{{ site.data.downloads.platforms.linux.checksum_url | relative_url }}">Download SHA256 checksum</a>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Run the AppImage</h2>
    <p>Download both files into the same directory, make the AppImage executable, verify it and run it.</p>
  </div>
  <pre><code>cd ~/Downloads
chmod +x Holder-0.2.0-dev-x86_64.AppImage
sha256sum -c Holder-0.2.0-dev-SHA256SUMS
./Holder-0.2.0-dev-x86_64.AppImage</code></pre>
  <div class="section-head following-copy">
    <p>
      This is an early preview of the next Holder release. The AppImage contains the desktop app and backend and
      starts its bundled backend when necessary. Your Holder data remains in your normal user data directory.
    </p>
    <p><strong>Preview note:</strong> back up your Holder data before trying a development build or switching between versions.</p>
    <p>Using Ubuntu? The <a href="{{ '/ubuntu/' | relative_url }}">Ubuntu PPA</a> remains the stable installation path.</p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Developer Build</h2>
    <p>
      To develop Holder on Linux or BSD, build the backend and desktop from source.
    </p>
    <p>The overall idea is:</p>
  </div>

  <pre><code>git clone https://github.com/HolderTeam/holder-daemon.git
git clone https://github.com/HolderTeam/holder-desktop.git

cd holder-daemon
./make.sh

cd ../holder-desktop
./make.sh</code></pre>

  <div class="section-head">
    <p>
      You will need to find the right development packages for your distribution. The exact package names vary by
      distribution. You need a C++20 toolchain, CMake, Ninja, Meson, Vala, GTK4, Libadwaita, GtkSourceView,
      libspelling, libsoup, libgit2, SQLite, OpenSSL, Boost, libsodium, and the other development libraries listed
      in the backend and desktop README files.
    </p>

    <p>Rather than looking everything up, the classic dependency whack-a-mole approach is often faster:</p>

  </div>

  <ol class="steps">
    <li>Run <code>./make.sh</code>.</li>
    <li>Let it compile until it moans loudly about something missing.</li>
    <li>Install that.</li>
    <li>Start again.</li>
    <li>Repeat until the compiler gives up complaining and builds Holder.</li>
  </ol>
</section>
