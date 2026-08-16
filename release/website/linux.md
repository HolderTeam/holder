---
layout: default
title: Holder for Linux
permalink: /linux/
---

<section class="page-panel">
  <p class="eyebrow">Linux</p>
  <h1>Holder for Linux</h1>
  <p class="lead">The easiest Linux path is Ubuntu using the HolderTeam PPA. Other distributions are currently source-build territory.</p>
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
      For other Linux distributions, build the backend and desktop from source. This assumes you are comfortable
      installing development packages for your distribution.
    </p>
  </div>

  <pre><code>git clone https://github.com/HolderTeam/holder-daemon.git
git clone https://github.com/HolderTeam/holder-desktop.git

cd holder-daemon
./make.sh

cd ../holder-desktop
./make.sh test
./make.sh run</code></pre>
</section>

<section class="section">
  <div class="section-head">
    <h2>Developer Dependencies</h2>
    <p>
      The exact package names vary by distribution. You need a C++20 toolchain, CMake, Ninja, Meson, Vala,
      GTK4, Libadwaita, GtkSourceView, libspelling, libsoup, libgit2, SQLite, OpenSSL, Boost, libsodium,
      and the other development libraries listed in the backend and desktop README files.
    </p>
    <p>
      If you get Holder building on another distribution, notes or packaging help are very welcome.
    </p>
  </div>
</section>
