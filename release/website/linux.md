---
layout: default
title: Holder for Linux
permalink: /linux/
---

<section class="page-panel">
  <p class="eyebrow">Linux</p>
  <h1>Holder for Linux and BSD</h1>
  <p class="lead">
    The easiest Linux path is Ubuntu using the HolderTeam PPA. Other distributions are currently source-build at the moment.
  </p>
</section>

<section class="section">
  <div class="section-head">
    <h2>Ubuntu Install</h2>
    <p>Install Holder from the HolderTeam Launchpad PPA.</p>
  </div>
  <pre><code>sudo add-apt-repository ppa:holderteam/holder
sudo apt update
sudo apt install holder</code></pre>
</section>

<section class="section">
  <div class="section-head">
    <h2>Developer Build</h2>
    <p>
      To develop holder on any Linux or BSD distribution, you build the backend and desktop from source.
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
