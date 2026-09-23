---
layout: default
title: Develop Holder on Windows
permalink: /develop/windows/
---

<section class="page-panel">
  <p class="eyebrow">Developer setup</p>
  <h1>Develop Holder on Windows</h1>
  <p class="lead">A fresh-machine path from Windows to a tested Holder backend and desktop app running together.</p>
</section>

<section class="section">
  <div class="section-head">
    <h2>What you will build</h2>
    <p>Holder has a platform-independent C++ core library, a native C++ backend, and a GTK desktop app. On Windows, Visual Studio builds the core and backend, while the MSYS2 UCRT64 environment builds the desktop app.</p>
    <p>Keep both programs running under the same Windows user account. The desktop discovers the local backend through a small per-user file written when the backend starts.</p>
  </div>
  <div class="grid three">
    <article class="card">
      <h3>Core library</h3>
      <p><a href="https://github.com/HolderTeam/holder-core">holder-core</a>, the reusable storage and domain library. Make core contributions in its own checkout.</p>
    </article>
    <article class="card">
      <h3>Backend</h3>
      <p><a href="https://github.com/HolderTeam/holder-daemon">holder-daemon</a>, built and tested with Visual Studio, CMake, Ninja, and vcpkg.</p>
    </article>
    <article class="card">
      <h3>Desktop</h3>
      <p><a href="https://github.com/HolderTeam/holder-desktop">holder-desktop</a>, built and tested in the MSYS2 UCRT64 shell with Meson and GTK.</p>
    </article>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Before you start</h2>
    <p>You need a 64-bit supported Windows installation, an internet connection, and several gigabytes of free disk space. The first vcpkg configure for each C++ repository downloads and compiles its dependencies; it can take a while and use roughly 2 GB per repository.</p>
    <p>This guide deliberately uses Visual Studio, not Visual Studio Code. Do not use an MSYS2 shell to build the backend, or a Visual Studio shell to build the GTK desktop app.</p>
  </div>
  <ol class="steps">
    <li>Install <a href="https://git-scm.com/download/win">Git for Windows</a>. Accept the normal installer defaults.</li>
    <li>Install <a href="https://visualstudio.microsoft.com/vs/community/">Visual Studio Community</a>. In its installer select <strong>Desktop development with C++</strong>; keep the CMake tools and a Windows SDK selected.</li>
    <li>Install <a href="https://www.msys2.org/">MSYS2</a> to its default location. From the Start menu open <strong>MSYS2 UCRT64</strong> (not MSYS, MINGW64, or CLANG64).</li>
  </ol>
  <pre><code>pacman -Syu</code></pre>
  <div class="section-head following-copy">
    <p>Run that update in UCRT64. If it asks you to close the shell, close it, reopen <strong>MSYS2 UCRT64</strong>, and run the command again until no further core update is required.</p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Get the source</h2>
    <p>Open PowerShell and make one directory for the three sibling repositories. Cloning the backend recursively also obtains the pinned copies it uses for normal backend builds.</p>
  </div>
  <pre><code>mkdir $HOME\src\holder
cd $HOME\src\holder
git clone --recurse-submodules https://github.com/HolderTeam/holder-daemon.git
git clone https://github.com/HolderTeam/holder-core.git
git clone https://github.com/HolderTeam/holder-desktop.git</code></pre>
  <div class="section-head following-copy">
    <p>The standalone <code>holder-core</code> checkout is the place to make and test core changes. Do not edit <code>holder-daemon\submodules\holder-core</code>: that is the backend's pinned dependency copy. If you cloned the backend without <code>--recurse-submodules</code>, fix it before building:</p>
  </div>
  <pre><code>cd $HOME\src\holder\holder-daemon
git submodule update --init --recursive</code></pre>
</section>

<section class="section">
  <div class="section-head">
    <h2>Build and test the core library</h2>
    <p>Open <code>holder-core</code> with Visual Studio using <strong>Open a local folder</strong>. Select the <code>windows-vcpkg-debug</code> configuration, choose <strong>Build</strong> → <strong>Build All</strong>, then use Test Explorer's <strong>Run All</strong> command.</p>
    <p>The core library has its own vcpkg installation directory, so its first build can be long even if you have already built the backend. A successful test run confirms that the canonical checkout is ready for core work.</p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Build and test the backend</h2>
    <p>Open Visual Studio, choose <strong>Open a local folder</strong>, and select the <code>holder-daemon</code> directory. Visual Studio recognises its CMake presets.</p>
  </div>
  <ol class="steps">
    <li>In the configuration drop-down, choose <code>windows-vcpkg-debug</code>.</li>
    <li>Choose <strong>Build</strong> → <strong>Build All</strong>. The first configure is expected to be slow while vcpkg builds dependencies. The finished executables are in <code>out\build\windows-vcpkg-debug</code>.</li>
    <li>Change the configuration to <code>windows-vcpkg-tests-debug</code> and choose <strong>Build</strong> → <strong>Build All</strong>.</li>
    <li>Choose <strong>Test</strong> → <strong>Run Test Preset for windows-vcpkg-tests-debug</strong> → <code>windows-vcpkg-tests-debug</code>.</li>
  </ol>
  <div class="section-head following-copy">
    <p>Success means the test run completes without failures. A small number of tests are intentionally skipped on Windows because they check POSIX-only behaviour.</p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Build and test the desktop app</h2>
    <p>Open a fresh <strong>MSYS2 UCRT64</strong> shell. The PowerShell path above is available there as <code>~/src/holder</code>.</p>
  </div>
  <pre><code>cd ~/src/holder/holder-desktop
./make-win.sh deps
./make-win.sh test</code></pre>
  <div class="section-head following-copy">
    <p>The first command installs the exact MSYS2 packages needed by the project. The test command configures <code>build-win</code>, builds the app, and runs its automated tests. Confirm that Meson reports no test failures.</p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Run both parts together</h2>
    <p>Use two terminals and leave the backend running while you start the desktop app.</p>
  </div>
  <div class="grid two">
    <article class="card">
      <h3>1. Start the backend in PowerShell</h3>
      <pre><code>cd $HOME\src\holder\holder-daemon
.\out\build\windows-vcpkg-debug\holderd.exe</code></pre>
      <p>It should report that it is listening on <code>127.0.0.1:11499</code> and print a local documentation URL. Leave this window open.</p>
    </article>
    <article class="card">
      <h3>2. Start the desktop in UCRT64</h3>
      <pre><code>cd ~/src/holder/holder-desktop
./make-win.sh run</code></pre>
      <p>Holder should open and show its initial Home project. Create a test card to confirm the desktop can reach the backend.</p>
    </article>
  </div>
  <div class="section-head following-copy">
    <p>For another quick check, open <a href="http://127.0.0.1:11499/docs">the local API documentation</a> while the backend is running. Stop it with <kbd>Ctrl</kbd>+<kbd>C</kbd> when finished.</p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Common first-machine problems</h2>
  </div>
  <div class="grid two">
    <article class="card">
      <h3>The backend configure looks stuck</h3>
      <p>The first vcpkg build can be long and quiet. Check Task Manager for active compiler processes and allow it to finish before assuming it has stopped.</p>
    </article>
    <article class="card">
      <h3>Meson cannot find GTK, Vala, or pkg-config</h3>
      <p>Close the terminal and reopen <strong>MSYS2 UCRT64</strong>, then rerun <code>./make-win.sh deps</code>. Packages installed for another MSYS2 shell are not the UCRT64 toolchain.</p>
    </article>
    <article class="card">
      <h3>The desktop says Holder is not running</h3>
      <p>Start <code>holderd.exe</code> first, from PowerShell under the same Windows account. Do not set <code>HOME</code> or <code>XDG_DATA_HOME</code> differently in either terminal.</p>
    </article>
    <article class="card">
      <h3>A step fails</h3>
      <p>Record the command, its complete output, your Windows version, and the Visual Studio and MSYS2 versions. That is enough information to turn the failure into a useful guide fix or issue.</p>
    </article>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Next steps</h2>
    <p>This is the platform setup guide, not a complete contribution policy. Each repository owns its current build commands and development notes: <a href="https://github.com/HolderTeam/holder-core#readme">core README</a>, <a href="https://github.com/HolderTeam/holder-daemon#readme">backend README</a>, and <a href="https://github.com/HolderTeam/holder-desktop#readme">desktop README</a>.</p>
    <p>When a backend change needs a core change, make and test the core change in the standalone repository first, commit it there, then advance the backend's <code>holder-core</code> submodule pointer to that commit. A future contributor guide can cover that shared GitHub and review workflow without duplicating this operating-system setup.</p>
  </div>
</section>
