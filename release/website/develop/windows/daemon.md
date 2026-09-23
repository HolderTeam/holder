---
layout: default
title: Develop Holder Daemon on Windows
permalink: /develop/windows/daemon/
---

<section class="page-panel">
  <p class="eyebrow">Windows developer setup · 2 of 3</p>
  <h1>Build Holder Daemon</h1>
  <p class="lead">Build and test the local Holder service that stores data and provides the command-line and web interfaces.</p>
</section>

<section class="section">
  <div class="section-head">
    <h2>Get the Tools</h2>
    <p>As explained in the <a href="{{ '/develop/windows/' | relative_url }}">Windows developer setup</a>, you need to install Git and create <code>$HOME\projects\holder</code>. You do not need to build Core first: Daemon gets and builds its own copy of the Core library.</p>
    <p>If you don't have it yet then install <a href="https://visualstudio.microsoft.com/vs/community/">Visual Studio Community</a>. Its installer groups tools into “workloads”; select <strong>Desktop development with C++</strong>, and accept the pre-selected CMake tools and Windows SDK.</p>
  </div>
</section>

<section class="section" id="download-daemon">
  <div class="section-head">
    <h2>Download the source code</h2>
    <p>Open PowerShell and run these commands. The extra <code>--recurse-submodules</code> argument downloads fixed copies of smaller projects that Daemon needs to build.</p>
  </div>
  <pre><code>cd $HOME\projects\holder
git clone --recurse-submodules https://github.com/HolderTeam/holder-daemon.git</code></pre>
  <div class="section-head following-copy">
    <p>Note 1: Do not edit <code>holder-daemon\submodules\holder-core</code>: it is Daemon's fixed copy of the core library. If you want to change the core logic, then make and test your changes in the separate <code>holder-core</code> folder.</p>
    <p>Note 2: If you cloned Daemon without <code>--recurse-submodules</code>, use these commands before building:</p>
  </div>
  <pre><code>cd $HOME\projects\holder\holder-daemon
git submodule update --init --recursive</code></pre>
</section>

<section class="section" id="build-daemon">
  <div class="section-head">
    <h2>Build and test</h2>
    <p>In Visual Studio, choose <strong>Open a local folder</strong> and select <code>holder-daemon</code>. Visual Studio reads the project's saved build choices automatically.</p>
  </div>
  <ol class="steps">
    <li>In the configuration drop-down, choose <code>windows-vcpkg-debug</code>. This is the normal, developer-friendly build.</li>
    <li>Choose <strong>Build</strong> → <strong>Build All</strong>. The first preparation step uses vcpkg to build Daemon's dependencies and is expected to be slow. When it finishes, the programs are in <code>out\build\windows-vcpkg-debug</code>.</li>
    <li>Change the configuration to <code>windows-vcpkg-tests-debug</code> and choose <strong>Build</strong> → <strong>Build All</strong>.</li>
    <li>Choose <strong>Test</strong> → <strong>Run Test Preset for windows-vcpkg-tests-debug</strong> → <code>windows-vcpkg-tests-debug</code>.</li>
  </ol>
  <div class="section-head following-copy">
    <p>You have succeeded when the test run completes without failures. A few tests are deliberately skipped on Windows because they check behaviour that only exists on Linux and similar systems.</p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>If a build seems stuck</h2>
    <p>The first vcpkg build can be long and quiet. Open Task Manager and check for active compiler processes before deciding that it has stopped.</p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Continue</h2>
    <p>You can now <a href="{{ '/develop/windows/desktop/' | relative_url }}">build the Holder desktop app</a>. When that page is complete, it shows how to start Daemon and Desktop together.</p>
    <p>If a Daemon change also needs a Core change, clone and test Core in its separate folder first. Then commit it there and update Daemon to use that saved version.</p>
  </div>
</section>
