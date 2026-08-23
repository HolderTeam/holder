---
layout: default
title: Holder for Ubuntu
permalink: /ubuntu/
---

<section class="page-panel">
  <p class="eyebrow">Ubuntu</p>
  <h1>Holder for Ubuntu</h1>
  <p class="lead">
    Install the stable Holder 0.1.7 release from the HolderTeam PPA. Ubuntu 24.04 is Holder's minimum supported Ubuntu release.
  </p>
</section>

<section class="section">
  <div class="section-head">
    <h2>Install from the PPA</h2>
    <p>The PPA keeps Holder integrated with Ubuntu's normal package installation and update tools.</p>
  </div>
  <pre><code>sudo add-apt-repository ppa:holderteam/holder
sudo apt update
sudo apt install holder</code></pre>
<p>It is the original, and arguably the best, way to install Holder. It runs natively on the system without any sandboxing or excess fluff.</p>
</section>

<section class="section">
  <div class="section-head">
    <h2>Also there is an AppImage</h2>
    <p>
      Holder is also available as a signed x86_64 AppImage, which aims to work on all distributions including Ubuntu.
      It is a preview rather than the current stable Ubuntu package and uses Holder's normal data directory.
      Back up your Holder data before trying the development build or switching between versions.
    </p>
  </div>
  <div class="button-row">
    <a class="button secondary" href="{{ '/linux/' | relative_url }}">See the AppImage preview</a>
  </div>
</section>
