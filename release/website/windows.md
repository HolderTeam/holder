---
layout: default
title: Holder for Windows
permalink: /windows/
---

<section class="page-panel">
  <p class="eyebrow">Windows</p>
  <h1>Holder for Windows</h1>
  <p class="lead">Download the Windows installer, or build Holder yourself while the signed release pipeline is being finished.</p>
  <div class="actions">
    <a class="button primary" data-download-asset="windows" href="https://github.com/HolderTeam/holder-release/releases/latest">Download for Windows</a>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Install</h2>
    <p>Download the latest Windows installer from the Holder release page and run it. The installer includes the desktop app, launcher, backend daemon and command-line tool.</p>
    # Show image 3 in connection with the next paragraph
    <p>When the installer starts, it is worth clicking yes to "Add Holder command line tools to PATH" because it allows you to use the command line tools which are useful in themselves, but especially if you have a problem.<p>
    # Show image 4 in connection with the next paragraph
    <p>The Installer copies the programs to your computer.</p>
    # show image 5 in connection with the next paragraph
    <p>When the installer finishes, tick Launch or select Holder from the start menu.</p>
 </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Usage</h2>
    # show image 6 in connection with the next paragraph
    <p>The window opens showing your projects and cards. By default you get a project called Home. Press the plus button to create new cards, press the folder with a plus to make new projects.</p>
    # show image 7 in connection with the next paragraph
    <p>Press the beaker icon to open the toolbox, press the keyboard icon to open the AI panel.</p>
    # show image 8 in connection with the next paragraph
    <p>The HTTP API is documented with Swagger.</p>
    # show image 9 in connection with the next paragraph
    <p>To access the command line interface, type holderctl into Powershell.</p>
    # show image 10 in connection with the next paragraph
    <p>Holder Framework Edition allows multiple ways to control your knowledge, and fits into all your ways of working.</p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Signed Release Builds Are Coming Soon</h2>
    <p>Holder is more transparent than almost anything else on your system. All of the source code is public, and the binaries are built using public GitHub workflows, so you can follow the whole process from source code to installer.</p>
    <p>You can also download the source code and build Holder yourself.</p>
    <p>However, getting those GitHub workflows to produce properly signed Windows installers has a few more hoops to jump through.</p>
    <p>It's good that now Windows takes software security seriously, but the developer experience for distributing traditional Windows applications is rather fragmented. Apple has Developer ID and notarization, while Linux distributions such as Ubuntu have established package-signing infrastructure. Windows has several different signing and distribution mechanisms that need to be fitted together to make a Github release process.</p>
    <p>This is slightly ironic given that GitHub itself is owned by Microsoft, but anyway.</p>
    <p>The goal is for every Holder release for Windows to be Authenticode-signed using a certificate that chains to a certificate authority trusted by Microsoft. Holder can then also go through the Microsoft Store certification process.</p>
    <p>I haven't finished getting all of that working yet. For now, you can either use one of the development builds or compile Holder yourself.</p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Staged Development Builds</h2>
    <p>Development builds are self-signed by the GitHub build workflow.</p>
    <p>Because the development signing certificate isn't trusted by Windows, Microsoft Defender SmartScreen will treat the installer as an unrecognised application and display:</p>
    <p><strong>Windows protected your PC</strong></p>
    # Add image 1
    <p>with a recommendation not to run the application.</p>
    <p>This does not mean that SmartScreen has detected malware in Holder. It means that Windows does not trust the certificate used to sign these development builds.</p>
    <p>If you've downloaded Holder from the official Holder project and want to continue, click <strong>More info</strong>, check that you're running the file you intended to download, and then choose <strong>Run anyway</strong>.</p>
    # Add image 2
    <p>If you regularly test or develop Holder on Windows, you can install and trust the Holder development certificate on your development machine. Windows can then verify subsequent development builds using that certificate without treating Holder's development signing identity as unknown.</p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Building From Source</h2>
    <p>Building from source on Windows uses Visual Studio for the backend and MSYS2 UCRT64 for the GTK desktop app.</p>
    <ul class="steps">
      <li>Install Visual Studio Community with the <strong>Desktop development with C++</strong> workload.</li>
      <li>Install <a href="https://www.msys2.org/">MSYS2</a> and open the <strong>UCRT64</strong> shell.</li>
      <li>Clone the backend: <code>git clone https://github.com/HolderTeam/holder-daemon.git</code>.</li>
      <li>Open <code>holder-daemon</code> in Visual Studio, choose <code>windows-vcpkg-debug</code>, then use <strong>Build All</strong>.</li>
      <li>For tests, choose <code>windows-vcpkg-tests-debug</code>, build, then run the CTest preset from Visual Studio's Test menu.</li>
      <li>Clone the desktop app: <code>git clone https://github.com/HolderTeam/holder-desktop.git</code>.</li>
      <li>In the MSYS2 UCRT64 shell, run <code>./make-win.sh deps</code>, then <code>./make-win.sh run</code>.</li>
    </ul>
    <p>The installer also includes a tiny launcher that starts the backend before opening the desktop app. You don't need to build it unless you are working on packaging, but you can get it here: <a href="https://github.com/HolderTeam/holder-launcher">holder-launcher</a>.</p>
  </div>
</section>
