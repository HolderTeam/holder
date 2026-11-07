---
layout: default
title: Develop Holder Core on Windows
permalink: /develop/windows/core/
---

<section class="page-panel">
  <p class="eyebrow">Windows developer setup · Core</p>
  <h1>Build Holder Core</h1>
  <p class="lead">Build and test the shared code that saves, finds, and organises Holder data.</p>
</section>

<section class="section">
  <div class="section-head">
    <h2>What is Holder Core?</h2>
    <p><a href="https://github.com/HolderTeam/holder/tree/main/core">Holder Core</a>, sometimes called libholder, is a library: reusable code that other programs use. Holder Daemon uses this library to work with your data. Core does not have an app window of its own.</p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Install the tools</h2>
    <ol class="steps">
      <li>Follow the <a href="{{ '/develop/windows/' | relative_url }}">Windows developer setup</a> to install Git and get a local checkout of Holder.</li>
      <li>Install <a href="https://visualstudio.microsoft.com/vs/community/">Visual Studio Community</a> (2022 or later) with the <strong>Desktop development with C++</strong> workload. Include the CMake tools, Windows SDK, and vcpkg. Both routes below use these tools.</li>
    </ol>
    <p>The first configuration can take a while. vcpkg downloads and builds the libraries Core depends on, which takes extra disk space.</p>
    <p>Choose either route to build and test Core.</p>
    <ul>
      <li><a href="#visual-studio">Visual Studio</a></li>
      <li><a href="#developer-powershell">Command line</a></li>
    </ul>
  </div>
</section>

<section class="section" id="visual-studio">
  <div class="section-head">
    <h2>Build and test in Visual Studio</h2>
  </div>
  <ol class="steps">
    <li>In Visual Studio, choose <strong>Open a local folder</strong> and select the <code>holder\core</code> folder.</li>
    <li>In the configuration drop-down, choose <code>windows-vcpkg-debug</code>. Wait for Visual Studio to finish preparing the project. You can follow its progress in the Output window.</li>
    <li>Choose <strong>Build</strong> → <strong>Build All</strong>. This project's saved build preset builds the library.</li>
    <li>Build the tests too: open <strong>View</strong> → <strong>Solution Explorer</strong>, switch to <strong>CMake Targets View</strong>, then right-click <code>holder_core_tests</code> and choose <strong>Build</strong>. A target is something the build tools produce; this one is the program that runs Core's tests.</li>
    <li>Open <strong>Test</strong> → <strong>Test Explorer</strong>, wait for the tests to appear, and choose <strong>Run All</strong>.</li>
  </ol>
  <div class="section-head following-copy">
    <p>If no tests appear, check that <code>holder_core_tests</code> finished building successfully.</p>
  </div>
</section>

<section class="section" id="developer-powershell">
  <div class="section-head">
    <h2>Build and test from the command line</h2>
    <p>Open the Windows Start menu, search for <strong>x64 Native Tools Command Prompt for VS</strong>, and open it. The window should say <strong>Environment initialized for: 'x64'</strong>.</p>
    <p>Run these commands one at a time in the Native Tools Command Prompt. If you cloned Holder somewhere else, replace the path on the first line with your Core folder.</p>
  </div>
  <p>Change directory to core:</p>
  <pre><code>cd /d "%USERPROFILE%\projects\holderteam\holder\core"</code></pre>
  <p>Describe this line:</p>  
  <pre><code>cmake --preset windows-vcpkg-debug</code></pre>
  <p>Describe this line:</p>
  <pre><code>cmake --build out/build/windows-vcpkg-debug --parallel</code></pre>
  <p>Describe this line:</p>
  <pre><code>ctest --test-dir out/build/windows-vcpkg-debug --output-on-failure --no-tests=error</code></pre>
  <div class="section-head following-copy">
    <p>Building the configured directory builds both the library and its tests. Confirm that CTest reports tests ran with no failures.</p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Continue</h2>
    <p>After changing Core, rebuild it and run the tests again. If a step fails, <a href="{{ '/develop/windows/' | relative_url }}#getting-help">ask for help</a> with the build or test output.</p>
    <p>You can now <a href="{{ '/develop/windows/' | relative_url }}#contributing">share your first change</a>. For more development details, see the <a href="https://github.com/HolderTeam/holder/tree/main/core#readme">Core README</a>.</p>
    <p>If you also want to work on another part of Holder, you can <a href="{{ '/develop/windows/daemon/' | relative_url }}">build the backend</a> or <a href="{{ '/develop/windows/desktop/' | relative_url }}">build the desktop app</a>.</p>
  </div>
</section>
