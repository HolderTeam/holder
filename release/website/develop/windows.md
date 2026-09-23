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
    <p>This guide is for people who want to change Holder's code. If you just want to use Holder, go to the <a href="{{ '/windows/' | relative_url }}">Holder for Windows</a> page to get the installer.</p>
    <p>You need a PC running 64-bit Windows 11 or Windows 10*, and an internet connection. Allow tens of gigabytes of free disk space for the development tools, downloaded libraries, and builds. The Visual Studio installer will show how much space your selected tools need; see <a href="https://learn.microsoft.com/en-us/visualstudio/releases/2026/vs-system-requirements">Microsoft's system requirements</a> for details.</p>
    <p>Install Git for Windows once: Git downloads project code and keeps track of the changes you make to it.</p>
  </div>
  <ol class="steps">
    <li>Install <a href="https://git-scm.com/download/win">Git for Windows</a>. Accept the normal installer defaults.</li>
    <li>Make a working directory.
      <ul>
        <li>We will keep the folders of Holder source code in one place.</li>
        <li>On my computer it is <code>C:\Users\pigeonwrath\projects\holderteam</code> but you can choose your own location, just be consistent.</li>
        <li>In PowerShell, <code>$HOME</code> means your Windows user folder. The commands below use your own folder automatically. If you choose a different workspace, use that location in each guide.</li>
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
    <p>Each Holder project has its own Git repository: its source files and the recorded history of changes. Source code is the text programmers change to alter how an app works.</p>
    <p>Choose the part you want to change. You do not need to complete all three guides.</p>
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
      <p><a href="https://github.com/HolderTeam/holder-desktop">holder-desktop</a> is the app with windows and buttons. Build it if you want to change the editors, navigation, or other parts of the user interface.</p>
      <p><a href="{{ '/develop/windows/desktop/' | relative_url }}">Build Desktop →</a></p>
    </article>
  </div>
</section>

<section class="section" id="getting-help">
  <div class="section-head">
    <h2>If you get stuck</h2>
    <p>Open the issue page for the part you are working on: <a href="https://github.com/HolderTeam/holder-core/issues">Core</a>, <a href="https://github.com/HolderTeam/holder-daemon/issues">Daemon</a>, or <a href="https://github.com/HolderTeam/holder-desktop/issues">Desktop</a>. Issues are GitHub's place for reporting problems and discussing fixes. Search for the error first; if no existing issue covers it, sign in to GitHub and open a new one.</p>
    <p>Include the guide you followed, the step that failed, and what you expected to happen. Copy the command and its output, or the error from Visual Studio's Output window. Include your Windows version and the versions of the tools you used. This helps someone else understand where you got stuck.</p>
  </div>
</section>

<section class="section" id="contributing">
  <div class="section-head">
    <h2>Share your first change</h2>
    <p>Once you can build and test the project, you are ready to try a small change. A documentation correction or a fix for an issue you can reproduce is a useful place to start.</p>
  </div>
  <ol class="steps">
    <li>Create a fork of the repository on GitHub: a copy under your own account where you can upload your changes. You can keep the local checkout you used for this guide and connect it to your fork.</li>
    <li>Create a branch for your change. A branch keeps that work separate from the project's main line of development.</li>
    <li>Make your change, rebuild, and run the tests described in the relevant guide. A commit records the changes you want to share, along with a short explanation.</li>
    <li>Push your commits to your fork, then open a pull request to the original HolderTeam repository. A pull request asks the maintainers to review and include your changes. Explain what changed and how you tested it, and complete the repository's contribution checklist.</li>
  </ol>
  <div class="section-head following-copy">
    <p>GitHub's <a href="https://docs.github.com/en/get-started/exploring-projects-on-github/contributing-to-a-project">step-by-step contribution guide</a> explains forks, branches, commits, and pull requests. Each Holder repository also has a <code>CLA.md</code> contributor agreement linked from its pull request checklist; read it before submitting your contribution.</p>
  </div>
</section>
