---
layout: default
title: Develop Holder Daemon on Windows
permalink: /develop/windows/daemon/
---

<section class="page-panel">
  <p class="eyebrow">Windows developer setup · Daemon</p>
  <h1>Build Holder Daemon</h1>
  <p class="lead">Build and test the local Holder service that stores data and provides the command-line and web interfaces.</p>
</section>

<section class="section">
  <div class="section-head">
    <h2>Install the tools</h2>
    <p>As explained in the <a href="{{ '/develop/windows/' | relative_url }}">Windows developer setup</a>, you need to install Git and create <code>$HOME\projects\holderteam</code>. You do not need to build Core separately: Daemon builds Core as part of its own build, from the Core folder you download in the next step.</p>
    <p>If you don't have it yet, install <a href="https://visualstudio.microsoft.com/vs/community/">Visual Studio Community</a> (2022 or later), the program with the purple logo. Visual Studio Code is a different program. The installer groups tools into “workloads”; select <strong>Desktop development with C++</strong>, and accept the pre-selected CMake tools and Windows SDK.</p>
  </div>
</section>

<section class="section" id="download-daemon">
  <div class="section-head">
    <h2>Download the source code</h2>
    <p>Open PowerShell and run these commands. The extra <code>--recurse-submodules</code> argument downloads fixed copies of smaller projects that Daemon needs to build.</p>
  </div>
  <pre><code>cd $HOME\projects\holderteam
git clone --recurse-submodules https://github.com/HolderTeam/holder-framework.git
git clone https://github.com/HolderTeam/holder-core.git</code></pre>
  <div class="section-head following-copy">
    <p>Daemon is the <code>daemon</code> folder inside <code>holder-framework</code>. The Windows debug build compiles Core from the separate <code>holder-core</code> folder next to it, so changes you make in Core are used the next time you build Daemon. To change Core's logic, follow the <a href="{{ '/develop/windows/core/' | relative_url }}">Core guide</a> and make and test your changes in that repository.</p>
    <p>If you cloned the framework without <code>--recurse-submodules</code>, use these commands before building:</p>
  </div>
  <pre><code>cd $HOME\projects\holderteam\holder-framework
git submodule update --init --recursive</code></pre>
</section>

<section class="section" id="build-daemon">
  <div class="section-head">
    <h2>Build and test</h2>
    <p>In Visual Studio, choose <strong>Open a local folder</strong> and select <code>holder-framework\daemon</code>. Visual Studio reads the project's saved build choices automatically.</p>
  </div>
  <ol class="steps">
    <li>In the configuration drop-down, choose <code>windows-vcpkg-debug</code>. A configuration is a saved set of build choices; this one builds the programs for development. Wait for Visual Studio to finish preparing the project. The first preparation step uses vcpkg to download and build the libraries Daemon needs, so it can be slow.</li>
    <li>Choose <strong>Build</strong> → <strong>Build All</strong>. When it finishes, look in <code>out\build\windows-vcpkg-debug</code> inside your Daemon folder for <code>holderd.exe</code>, the backend, and <code>holderctl.exe</code>, the command-line tool.</li>
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
    <p>If Visual Studio reports an error, copy it from the Output window and <a href="{{ '/develop/windows/' | relative_url }}#getting-help">ask for help</a>.</p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Continue</h2>
    <p>You can now <a href="{{ '/develop/windows/' | relative_url }}#contributing">share your first change</a>. The <a href="https://github.com/HolderTeam/holder-framework/tree/main/daemon#readme">Daemon README</a> has more development details.</p>
    <p>If you also want to work on the user interface, <a href="{{ '/develop/windows/desktop/' | relative_url }}">build the Holder desktop app</a>. That guide also shows how to start Daemon and Desktop together.</p>
    <p>If a Daemon change also needs a Core change, make and test it in the Core folder first, then commit it there.</p>
  </div>
</section>
