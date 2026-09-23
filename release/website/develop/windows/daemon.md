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
    <h2>Start here if you completed Core</h2>
    <p>You already have Git, Visual Studio, and <code>$HOME\src\holder</code>. Continue with <a href="#download-daemon">Download Daemon</a>; there is nothing to install again.</p>
    <p>Starting from a new PC instead? Complete <a href="{{ '/develop/windows/core/' | relative_url }}">Build Holder Core</a> first. It installs the shared tools and makes the Holder workspace this page expects.</p>
  </div>
</section>

<section class="section" id="download-daemon">
  <div class="section-head">
    <h2>Download Daemon</h2>
    <p>Open PowerShell and run these commands. The extra <code>--recurse-submodules</code> part downloads fixed copies of smaller projects that Daemon needs to build.</p>
  </div>
  <pre><code>cd $HOME\src\holder
git clone --recurse-submodules https://github.com/HolderTeam/holder-daemon.git</code></pre>
  <div class="section-head following-copy">
    <p>Do not edit <code>holder-daemon\submodules\holder-core</code>: it is Daemon's fixed copy of Core. Make and test Core changes in the separate <code>holder-core</code> folder. If you cloned Daemon without <code>--recurse-submodules</code>, use these commands before building:</p>
  </div>
  <pre><code>cd $HOME\src\holder\holder-daemon
git submodule update --init --recursive</code></pre>
</section>

<section class="section" id="build-daemon">
  <div class="section-head">
    <h2>Build and test Daemon</h2>
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
    <p>Next, <a href="{{ '/develop/windows/desktop/' | relative_url }}">build the Holder desktop app</a>. When that page is complete, it shows how to start Daemon and Desktop together.</p>
    <p>If a backend change also needs a Core change, make and test the Core change in its separate folder first. Then commit it there and update Daemon to use that saved version.</p>
  </div>
</section>
