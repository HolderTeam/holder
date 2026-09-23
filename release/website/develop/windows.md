---
layout: default
title: Develop Holder on Windows
permalink: /develop/windows/
---

<section class="page-panel">
  <p class="eyebrow">Developer setup</p>
  <h1>Develop Holder on Windows</h1>
  <p class="lead">Set up a Windows computer to contribute Holder code.</p>
</section>

<section class="section">
  <div class="section-head">
    <h2>Before you begin</h2>
    <p>This is for aspiring developers, if you just want to use Holder go to the <a href="{{ '/windows/' | relative_url }}">Holder for Windows</a> page to get the installer.</p>
    <p>You need a PC running Windows 11 or Windows 10*, an internet connection, and several gigabytes of free disk space. Install Git for Windows once: Git downloads project code and keeps track of the changes you make to it.</p>
  </div>
  <ol class="steps">
    <li>Install <a href="https://git-scm.com/download/win">Git for Windows</a>. Accept the normal installer defaults.</li>
    <li>Make a working directory.
      <ul>
        <li>We will keep the folders of Holder source code in one place.</li>
        <li>On my computer it is <code>C:\Users\pigeonwrath\projects\holderteam</code> but you can choose your own location, just be consistent.</li>
        <li>Open PowerShell, Windows’ built-in command window, and run these commands once:</li>
      </ul>
  <pre><code>mkdir $HOME\projects\holderteam
cd $HOME\projects\holderteam</code></pre>
  </li></ol>

  <p class="footnote"><sup>*</sup> Windows 11 is 64-bit only. Windows 10 also has 32-bit editions. The Core and Daemon guides use 64-bit build tools, but advanced users may be able to adapt them to build <code>holder-core</code> and <code>holder-daemon</code> for x86, then use <code>holderctl</code> or the local HTTP API. There is no current 32-bit holder-desktop build path.</p>

</section>

<section class="section">
  <div class="section-head">
    <h2>Three parts, three small guides</h2>
    <p>Holder is made up of subprojects. A subproject, known as a repository, is simply a folder of source code: the files that programmers change to alter how an app works.</p>
    <p>The core and backend use Visual Studio. The desktop app uses MSYS2 UCRT64 instead.</p>
  </div>
  <div class="grid three">
    <article class="card">
      <h3>Core library</h3>
      <p><a href="https://github.com/HolderTeam/holder-core">holder-core</a> contains the shared code for saving, finding, and organising Holder data.</p>
      <p><a href="{{ '/develop/windows/core/' | relative_url }}">Build Core →</a></p>
    </article>
    <article class="card">
      <h3>Backend</h3>
      <p><a href="https://github.com/HolderTeam/holder-daemon">holder-daemon</a> is the local service that stores your data and provides Holder's command-line and web interfaces.</p>
      <p><a href="{{ '/develop/windows/daemon/' | relative_url }}">Build Daemon →</a></p>
    </article>
    <article class="card">
      <h3>Desktop</h3>
      <p><a href="https://github.com/HolderTeam/holder-desktop">holder-desktop</a> is the GUI application. Build it if you want to make changes to the windows, buttons, editors, navigation, and so on.</p>
      <p><a href="{{ '/develop/windows/desktop/' | relative_url }}">Build Desktop →</a></p>
    </article>
  </div>
</section>
