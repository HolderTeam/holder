---
layout: default
title: Develop Holder Desktop on Windows
permalink: /develop/windows/desktop/
---

<section class="page-panel">
  <p class="eyebrow">Windows developer setup · 3 of 3</p>
  <h1>Build Holder Desktop</h1>
  <p class="lead">Build and test the Holder windowed app, then run it with your local backend.</p>
</section>

<section class="section">
  <div class="section-head">
    <h2>Before you build Desktop</h2>
    <p>First complete the shared <a href="{{ '/develop/windows/' | relative_url }}">Windows developer setup</a> to install Git and create <code>$HOME\src\holder</code>. You can build and test Desktop on its own.</p>
    <p>To run Desktop after building it, you will also need a built Daemon. If you do not have one yet, follow <a href="{{ '/develop/windows/daemon/' | relative_url }}">Build Holder Daemon</a> when you reach the run step below.</p>
  </div>
</section>

<section class="section" id="install-msys2">
  <div class="section-head">
    <h2>Install MSYS2</h2>
    <p>The desktop uses GTK, a set of tools for making application windows. MSYS2 supplies GTK and the tools needed to build the desktop app.</p>
  </div>
  <ol class="steps">
    <li>Install <a href="https://www.msys2.org/">MSYS2</a> to its default location.</li>
    <li>From the Start menu, open <strong>MSYS2 UCRT64</strong>, not MSYS, MINGW64, or CLANG64.</li>
  </ol>
  <pre><code>pacman -Syu</code></pre>
  <div class="section-head following-copy">
    <p>Copy that command into the UCRT64 window and press Enter. It updates MSYS2 itself. If it asks you to close the window, do so, reopen <strong>MSYS2 UCRT64</strong>, and run the same command again until it no longer asks for a core update.</p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Download Desktop</h2>
    <p>Open PowerShell and run these commands to download the desktop project into the Holder workspace you made on the Core page.</p>
  </div>
  <pre><code>cd $HOME\src\holder
git clone https://github.com/HolderTeam/holder-desktop.git</code></pre>
</section>

<section class="section">
  <div class="section-head">
    <h2>Build and test Desktop</h2>
    <p>Open a new <strong>MSYS2 UCRT64</strong> window. This kind of command window is often called a shell. The folder you made in PowerShell is called <code>~/src/holder</code> here; <code>~</code> means your own home folder.</p>
  </div>
  <pre><code>cd ~/src/holder/holder-desktop
./make-win.sh deps
./make-win.sh test</code></pre>
  <div class="section-head following-copy">
    <p>The first command installs the MSYS2 tools and libraries the desktop needs. The second prepares the <code>build-win</code> folder, builds the app, and runs its automated checks. Wait for it to finish and confirm that Meson reports no test failures.</p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Run Desktop and Daemon together</h2>
    <p>Daemon is a local service: it keeps running in one command window while the desktop app talks to it in another. Keep both under the same Windows user account.</p>
  </div>
  <div class="grid two">
    <article class="card">
      <h3>1. Start Daemon in PowerShell</h3>
      <pre><code>cd $HOME\src\holder\holder-daemon
.\out\build\windows-vcpkg-debug\holderd.exe</code></pre>
      <p>It should say that it is listening on <code>127.0.0.1:11499</code> and print a local documentation address. That address works only on your own PC. Leave this window open.</p>
    </article>
    <article class="card">
      <h3>2. Start Desktop in UCRT64</h3>
      <pre><code>cd ~/src/holder/holder-desktop
./make-win.sh run</code></pre>
      <p>Holder should open and show its first Home project. Create a test card. If it appears, Desktop and Daemon are talking to each other.</p>
    </article>
  </div>
  <div class="section-head following-copy">
    <p>For another check, open <a href="http://127.0.0.1:11499/docs">the local API documentation</a> while Daemon is running. An API is a way for programs to ask another program to do something; this page describes Holder's local API. Stop Daemon with <kbd>Ctrl</kbd>+<kbd>C</kbd> when you have finished.</p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Common first-machine problems</h2>
  </div>
  <div class="grid two">
    <article class="card">
      <h3>Meson cannot find GTK, Vala, or pkg-config</h3>
      <p>Close the window, reopen <strong>MSYS2 UCRT64</strong>, and run <code>./make-win.sh deps</code> again. Packages installed for a different kind of MSYS2 window are not available to UCRT64.</p>
    </article>
    <article class="card">
      <h3>Desktop says Holder is not running</h3>
      <p>Start <code>holderd.exe</code> first, from PowerShell under the same Windows account. Do not change <code>HOME</code> or <code>XDG_DATA_HOME</code> in either window; they are advanced settings that tell programs where to look for their files.</p>
    </article>
    <article class="card">
      <h3>A step fails</h3>
      <p>Copy the command and every line it printed, then note your Windows version and the Visual Studio and MSYS2 versions. This gives someone else enough information to reproduce the problem and improve the guide.</p>
    </article>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>You are ready</h2>
    <p>You can now change code, rebuild the part you changed, and run your local Holder app to see the result. Each project keeps its detailed and up-to-date development notes in its own README: <a href="https://github.com/HolderTeam/holder-core#readme">Core</a>, <a href="https://github.com/HolderTeam/holder-daemon#readme">Daemon</a>, and <a href="https://github.com/HolderTeam/holder-desktop#readme">Desktop</a>.</p>
  </div>
</section>

<p class="footnote"><sup>*</sup> Windows 11 is 64-bit only. Windows 10 also has 32-bit editions. This desktop setup uses 64-bit build tools and cannot be used on them. Advanced users may still be able to build <code>holder-core</code> and <code>holder-daemon</code> for x86, then use <code>holderctl</code> or the local HTTP API; there is no current 32-bit desktop build path.</p>
