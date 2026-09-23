---
layout: default
title: Develop Holder Core on Windows
permalink: /develop/windows/core/
---

<section class="page-panel">
  <p class="eyebrow">Windows developer setup · 1 of 3</p>
  <h1>Build Holder Core</h1>
  <p class="lead">Start here. You will install the tools for Holder's C++ code, download the core project, and run its tests.</p>
</section>

<section class="section">
  <div class="section-head">
    <h2>What is Core?</h2>
    <p><a href="https://github.com/HolderTeam/holder-core">holder-core</a> contains the shared code that saves, finds, and organises Holder data. The backend uses it, and future Holder apps can use it too.</p>
    <p>You need a PC running Windows 11 or Windows 10*, an internet connection, and several gigabytes of free disk space. This guide uses Visual Studio, the large purple application, not Visual Studio Code, the smaller blue editor.</p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Install the tools</h2>
    <p>Git downloads and keeps track of source code. Visual Studio turns C++ source code into programs that Windows can run.</p>
  </div>
  <ol class="steps">
    <li>Install <a href="https://git-scm.com/download/win">Git for Windows</a>. Accept the normal installer defaults.</li>
    <li>Install <a href="https://visualstudio.microsoft.com/vs/community/">Visual Studio Community</a>. Its installer groups tools into “workloads”; select <strong>Desktop development with C++</strong>, and leave the selected CMake tools and Windows SDK alone.</li>
  </ol>
</section>

<section class="section">
  <div class="section-head">
    <h2>Download Core</h2>
    <p>Open PowerShell, Windows’ built-in command window, and copy these commands in one at a time. They make a home for your Holder projects, then download Core into it. “Clone” is Git’s word for making your own local copy of a project.</p>
  </div>
  <pre><code>mkdir $HOME\src\holder
cd $HOME\src\holder
git clone https://github.com/HolderTeam/holder-core.git</code></pre>
</section>

<section class="section">
  <div class="section-head">
    <h2>Build and test Core</h2>
    <p>In Visual Studio, choose <strong>Open a local folder</strong> and select the <code>holder-core</code> folder. A configuration is a saved set of build choices. Choose <code>windows-vcpkg-debug</code>, then choose <strong>Build</strong> → <strong>Build All</strong>. Finally, open Test Explorer and choose <strong>Run All</strong>.</p>
    <p>The first build uses vcpkg, a helper that downloads and builds code Holder depends on. It can take a while and use roughly 2 GB. When the tests finish without failures, your Core setup is ready.</p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Continue</h2>
    <p>Next, <a href="{{ '/develop/windows/daemon/' | relative_url }}">build the Holder backend</a>. You already have Git, Visual Studio, and the Holder workspace, so begin by downloading Daemon; there is nothing to install again.</p>
  </div>
</section>

<p class="footnote"><sup>*</sup> Windows 11 is 64-bit only. Windows 10 also has 32-bit editions. This guide uses 64-bit build tools, but advanced users may be able to adapt it to build <code>holder-core</code> for x86.</p>
