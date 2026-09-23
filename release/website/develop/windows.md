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
      <p><a href="{{ '/develop/windows/daemon/' | relative_url }}">Then build Daemon →</a></p>
    </article>
    <article class="card">
      <h3>Desktop</h3>
      <p><a href="https://github.com/HolderTeam/holder-desktop">holder-desktop</a> is the windowed app that you use day to day.</p>
      <p><a href="{{ '/develop/windows/desktop/' | relative_url }}">Finally build Desktop →</a></p>
    </article>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Choose your starting point</h2>
    <p>New to Holder development? Begin with Core. It installs Git and Visual Studio, creates your Holder workspace, and gives you a first successful build.</p>
    <p>Already have a Core checkout? Go straight to Daemon or Desktop. Each page says exactly what it assumes you have already done.</p>
  </div>
</section>

<p class="footnote"><sup>*</sup> Windows 11 is 64-bit only. Windows 10 also has 32-bit editions. The Core and Daemon guides use 64-bit build tools, but advanced users may be able to adapt them to build <code>holder-core</code> and <code>holder-daemon</code> for x86, then use <code>holderctl</code> or the local HTTP API. There is no current 32-bit desktop build path.</p>
