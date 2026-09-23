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
    <p><a href="https://github.com/HolderTeam/holder-core">holder-core</a>, sometimes called libholder, is a library: reusable code that other programs use. Holder Daemon uses this library to work with your data. Core does not have an app window of its own.</p>
    <p>These instructions use <strong>Visual Studio Community</strong>, which has a purple logo. Visual Studio Code, with the blue logo, is a different program.</p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Install the tools</h2>
    <ol class="steps">
      <li>As explained in the <a href="{{ '/develop/windows/' | relative_url }}">Windows developer setup</a>, you need to install <a href="https://git-scm.com/download/win">Git</a> and create <code>$HOME\projects\holderteam</code>.</li>
      <li>Install <a href="https://visualstudio.microsoft.com/vs/community/">Visual Studio Community</a> (2022 or later). It includes tools that turn C++ source code into programs Windows can run. Its installer groups tools into “workloads”; select <strong>Desktop development with C++</strong>, and accept the pre-selected CMake tools and Windows SDK.</li>
    </ol>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Download the source code</h2>
    <p>Open PowerShell and run these commands, one line at a time. “Clone” is Git’s word for making your own local copy of a project.</p>
  </div>
  <pre><code>cd $HOME\projects\holderteam
git clone https://github.com/HolderTeam/holder-core.git</code></pre>
</section>

<section class="section">
  <div class="section-head">
    <h2>Build and test Core</h2>
    <p>A configuration is a saved set of build choices. The first preparation step uses vcpkg, a helper that downloads and builds code Holder depends on. This can take a while and needs additional disk space beyond the Visual Studio installation.</p>
  </div>
  <ol class="steps">
    <li>In Visual Studio, choose <strong>Open a local folder</strong> and select the <code>holder-core</code> folder you just downloaded.</li>
    <li>In the configuration drop-down, choose <code>windows-vcpkg-debug</code>. Wait for Visual Studio to finish preparing the project. You can follow its progress in the Output window.</li>
    <li>Choose <strong>Build</strong> → <strong>Build All</strong>. This project's saved build preset builds the library.</li>
    <li>Build the tests too: open <strong>View</strong> → <strong>Solution Explorer</strong>, switch to <strong>CMake Targets View</strong>, then right-click <code>holder_core_tests</code> and choose <strong>Build</strong>. A target is something the build tools produce; this one is the program that runs Core's tests.</li>
    <li>Open <strong>Test</strong> → <strong>Test Explorer</strong>, wait for the tests to appear, and choose <strong>Run All</strong>.</li>
  </ol>
  <div class="section-head following-copy">
    <p>If no tests appear, check that <code>holder_core_tests</code> finished building successfully. An empty test list does not mean the tests passed. If you are stuck, <a href="{{ '/develop/windows/' | relative_url }}#getting-help">ask for help</a> with the build output.</p>
    <p>When the tests finish without failures, you are ready to work on Holder Core. You can now change the code that controls how Holder stores, finds, and organises data, rebuild it, and run the tests again to check your changes.</p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Continue</h2>
    <p>You can now <a href="{{ '/develop/windows/' | relative_url }}#contributing">share your first change</a>. For more development details, see the <a href="https://github.com/HolderTeam/holder-core#readme">Core README</a>.</p>
    <p>If you also want to work on another part of Holder, you can <a href="{{ '/develop/windows/daemon/' | relative_url }}">build the backend</a> or <a href="{{ '/develop/windows/desktop/' | relative_url }}">build the desktop app</a>.</p>
  </div>
</section>
