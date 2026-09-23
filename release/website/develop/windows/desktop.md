---
layout: default
title: Develop Holder Desktop on Windows
permalink: /develop/windows/desktop/
---

<section class="page-panel">
  <p class="eyebrow">Windows developer setup · Desktop</p>
  <h1>Build Holder Desktop</h1>
  <p class="lead">Build and test the Holder windowed app, then run it with your local backend.</p>
</section>

<section class="section">
  <div class="section-head">
    <h2>Install the tools</h2>
    <p>As explained in the <a href="{{ '/develop/windows/' | relative_url }}">Windows developer setup</a>, you need to install Git and create <code>$HOME\projects\holderteam</code>.</p>
    <p>Holder Desktop needs Holder Daemon, the local backend, to read and save your data.</p>
    <p>If you only want to change the desktop app, get the backend from the <a href="{{ '/windows/' | relative_url }}">Windows installer</a>. If you want to change the backend too, <a href="{{ '/develop/windows/daemon/' | relative_url }}">build your own Holder Daemon</a>. You do not need Visual Studio or a separate Core checkout just to build Desktop.</p>
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
    <p>Open PowerShell and run these commands to download the desktop project into the workspace you made in the <a href="{{ '/develop/windows/' | relative_url }}">Windows developer setup</a>.</p>
  </div>
  <pre><code>cd $HOME\projects\holderteam
git clone https://github.com/HolderTeam/holder-desktop.git</code></pre>
</section>

<section class="section">
  <div class="section-head">
    <h2>Build and test Desktop</h2>
    <p>Open a new <strong>MSYS2 UCRT64</strong> window. This kind of command window is often called a shell. Your source files stay in the same Windows folder, but MSYS2 writes paths differently: for example, <code>C:\Users\pigeonwrath</code> becomes <code>/c/Users/pigeonwrath</code>.</p>
    <p>MSYS2 normally has its own home folder, so <code>~</code> may not point to the Windows user folder you used in PowerShell. The command below uses <code>USERPROFILE</code> to find your Windows user folder and <code>cygpath</code> to convert its path for MSYS2. You can copy it as written if you used the suggested workspace:</p>
  </div>
  <pre><code>cd "$(cygpath -u "$USERPROFILE")/projects/holderteam/holder-desktop"</code></pre>
  <div class="section-head following-copy">
    <p>If you chose a different workspace, use its path instead. For example, a checkout at <code>D:\projects\holderteam\holder-desktop</code> would use <code>cd /d/projects/holderteam/holder-desktop</code>. See <a href="https://www.msys2.org/docs/filesystem-paths/">MSYS2's path guide</a> for more examples.</p>
    <p>Once you are in the Desktop folder, run these commands one at a time:</p>
  </div>
  <pre><code>./make-win.sh deps
./make-win.sh test</code></pre>
  <div class="section-head following-copy">
    <p>The first command installs the MSYS2 tools and libraries the desktop needs. The second uses Meson, the desktop's build tool, to prepare the <code>build-win</code> folder, build the app, and run its automated checks. Wait for it to finish and confirm that tests ran with no failures.</p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Run Desktop and Daemon together</h2>
    <p>Daemon is a local service: it keeps running in one command window while the desktop app talks to it in another. Keep both under the same Windows user account. First, start the backend using the instructions for the route you chose.</p>
    <p>If a backend is already running, you can use it and skip starting another copy. Close any open Desktop window before starting the app you just built.</p>
  </div>
  <div class="grid two">
    <article class="card">
      <h3>If you installed the backend</h3>
      <p>Open PowerShell and run this command for the default installation folder:</p>
      <pre><code>&amp; "$env:LOCALAPPDATA\Programs\Holder\bin\holderd.exe"</code></pre>
      <p>If you chose another installation folder, replace the path with the location of <code>bin\holderd.exe</code> inside that folder. The <code>&amp;</code> tells PowerShell to run the program at the quoted path.</p>
    </article>
    <article class="card">
      <h3>If you built the backend</h3>
      <p>Open PowerShell and run:</p>
      <pre><code>cd $HOME\projects\holderteam\holder-daemon
.\out\build\windows-vcpkg-debug\holderd.exe</code></pre>
    </article>
  </div>
  <div class="section-head following-copy">
    <p>Daemon should say that it is listening on <code>127.0.0.1:11499</code> and print a local documentation address. That address works only on your own PC. Leave the backend window open.</p>
    <h3>Start your Desktop build in UCRT64</h3>
    <p>In an <strong>MSYS2 UCRT64</strong> window, run:</p>
  </div>
  <pre><code>cd "$(cygpath -u "$USERPROFILE")/projects/holderteam/holder-desktop"
./make-win.sh run</code></pre>
  <div class="section-head following-copy">
    <p>Holder should open with your projects, or a Home project if this is your first run. Create a test card. If it appears, Desktop and Daemon are talking to each other.</p>
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
      <p>Close the window, reopen <strong>MSYS2 UCRT64</strong>, and use the <code>cd</code> command above to return to your Desktop folder. Run <code>./make-win.sh deps</code>, then <code>./make-win.sh test</code> again. Packages installed for a different kind of MSYS2 window are not available to UCRT64.</p>
    </article>
    <article class="card">
      <h3>Desktop says Holder is not running</h3>
      <p>Start <code>holderd.exe</code> first, from PowerShell under the same Windows account. Do not change <code>HOME</code> or <code>XDG_DATA_HOME</code> in either window; they are advanced settings that tell programs where to look for their files.</p>
    </article>
    <article class="card">
      <h3>A step fails</h3>
      <p><a href="{{ '/develop/windows/' | relative_url }}#getting-help">Ask for help on GitHub</a>. Include the step you followed, the command and its output, your Windows version, and the versions of the tools you used.</p>
    </article>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>You are ready</h2>
    <p>You can now change code, rebuild the part you changed, and run your local Holder app to see the result. Each project keeps its detailed and up-to-date development notes in its own README: <a href="https://github.com/HolderTeam/holder-core#readme">Core</a>, <a href="https://github.com/HolderTeam/holder-daemon#readme">Daemon</a>, and <a href="https://github.com/HolderTeam/holder-desktop#readme">Desktop</a>.</p>
    <p>When you are happy with your change and the tests pass, follow <a href="{{ '/develop/windows/' | relative_url }}#contributing">Share your first change</a> to submit it for review.</p>
  </div>
</section>
