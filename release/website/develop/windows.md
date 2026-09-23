---
layout: default
title: Develop Holder on Windows
permalink: /develop/windows/
---

<section class="page-panel">
  <p class="eyebrow">Developer setup</p>
  <h1>Develop Holder on Windows</h1>
  <p class="lead">Set up a Windows computer to work on Holder, one part at a time.</p>
</section>

<section class="section">
  <div class="section-head">
    <h2>Before you choose a project</h2>
    <p>You need a PC running Windows 11 or Windows 10*, an internet connection, and several gigabytes of free disk space. Install Git for Windows once: Git downloads project code and keeps track of the changes you make to it.</p>
  </div>
  <ol class="steps">
    <li>Install <a href="https://git-scm.com/download/win">Git for Windows</a>. Accept the normal installer defaults.</li>
  </ol>
  <div class="section-head following-copy">
    <p>We will keep the three Holder project folders together in <code>$HOME\src\holder</code>. Open PowerShell, Windows’ built-in command window, and run these commands once:</p>
  </div>
  <pre><code>mkdir $HOME\src\holder
cd $HOME\src\holder</code></pre>
</section>

<section class="section">
  <div class="section-head">
    <h2>Three parts, three small guides</h2>
    <p>Holder is made from three projects. A project is simply a folder of source code: the files that programmers change to alter how an app works.</p>
    <p>The core and backend use Visual Studio. The desktop app uses MSYS2 UCRT64 instead. That split is normal for Holder, but it is too much to learn all at once, so build one part before moving to the next.</p>
  </div>
  <div class="grid three">
    <article class="card">
      <h3>Core library</h3>
      <p><a href="https://github.com/HolderTeam/holder-core">holder-core</a> contains the shared code for saving, finding, and organising Holder data. Change it in its own folder.</p>
      <p><a href="{{ '/develop/windows/core/' | relative_url }}">Start with Core →</a></p>
    </article>
    <article class="card">
      <h3>Backend</h3>
      <p><a href="https://github.com/HolderTeam/holder-daemon">holder-daemon</a> is the local service that stores your data and provides Holder's command-line and web interfaces.</p>
      <p><a href="{{ '/develop/windows/daemon/' | relative_url }}">Build Daemon →</a></p>
    </article>
    <article class="card">
      <h3>Desktop</h3>
      <p><a href="https://github.com/HolderTeam/holder-desktop">holder-desktop</a> is the windowed app that you use day to day.</p>
      <p><a href="{{ '/develop/windows/desktop/' | relative_url }}">Build Desktop →</a></p>
    </article>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Choose your starting point</h2>
    <p>New to programming? Core is a gentle first build. You can also start with Daemon or Desktop when that is the part you want to work on.</p>
    <p>Each project page tells you which additional tools it needs. You do not need to finish the other project pages first.</p>
  </div>
</section>

<p class="footnote"><sup>*</sup> Windows 11 is 64-bit only. Windows 10 also has 32-bit editions. The Core and Daemon guides use 64-bit build tools, but advanced users may be able to adapt them to build <code>holder-core</code> and <code>holder-daemon</code> for x86, then use <code>holderctl</code> or the local HTTP API. There is no current 32-bit desktop build path.</p>
