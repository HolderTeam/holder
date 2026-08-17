---
layout: default
title: Holder for Windows
permalink: /windows/
---

<section class="page-panel">
  <p class="eyebrow">Windows</p>
  <h1>Holder for Windows</h1>
  <p class="lead">Download the Windows installer, or build Holder yourself while the Windows signing pipeline is being finished.</p>
  <div class="actions">
    <a class="button primary" data-download-asset="windows" href="https://github.com/HolderTeam/holder-release/releases/latest">Download for Windows</a>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Install</h2>
    <p>Download the <a href="https://github.com/HolderTeam/holder-release/releases/download/holder-v0.1.7/Holder-windows-0.1.7-dev-Setup.exe">latest Windows installer</a> from the <a href="https://github.com/HolderTeam/holder-release/releases">Holder release page</a> and run it. The installer includes the desktop app, launcher, backend daemon and command-line tool.</p>
    <p>When the installer starts, it is worth selecting <strong>Add Holder command line tools to PATH</strong>. This lets you run <code>holderctl</code> from PowerShell, which is useful both for normal command-line use and for fixing problems.</p>
    <figure class="page-image">
      <img src="{{ '/assets/img/platforms/windows/win-3-installer-1.png' | relative_url }}" alt="The Holder installer showing setup options including adding Holder command line tools to PATH">
    </figure>
    <p>The installer copies the Holder programs and supporting files to your computer.</p>
    <figure class="page-image">
      <img src="{{ '/assets/img/platforms/windows/win-4-installer-2.png' | relative_url }}" alt="The Holder installer copying files">
    </figure>
    <p>When the installer finishes, leave <strong>Launch Holder</strong> selected, or open Holder later from the Start menu.</p>
    <figure class="page-image">
      <img src="{{ '/assets/img/platforms/windows/win-5-installer-3-done-launch.png' | relative_url }}" alt="The Holder installer completion screen with the launch option selected">
    </figure>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Usage</h2>
    <p>Holder opens with your projects and cards. By default you get a project called Home. Press the plus button to create a card, or the folder-plus button to create a project.</p>
    <figure class="page-image">
      <img src="{{ '/assets/img/platforms/windows/win-6-installer-2-done-launch.png' | relative_url }}" alt="Holder running on Windows with the default Home project open">
    </figure>
    <p>Press the beaker icon to open the toolbox, press the keyboard icon to open the AI panel.</p>
    <figure class="page-image">
      <img src="{{ '/assets/img/platforms/windows/win-7-installer-3-done-launch.png' | relative_url }}" alt="Holder running on Windows with the toolbox and AI panel visible">
    </figure>
    <p>The local HTTP API is documented with Swagger UI.</p>
    <figure class="page-image">
      <img src="{{ '/assets/img/platforms/windows/win-8-webapi-docs.png' | relative_url }}" alt="Holder API documentation open in a browser on Windows">
    </figure>
    <p>To access the command-line interface, type <code>holderctl</code> into PowerShell.</p>
    <figure class="page-image">
      <img src="{{ '/assets/img/platforms/windows/win-9-holderctl.png' | relative_url }}" alt="holderctl running in PowerShell on Windows">
    </figure>
    <p>Holder Framework Edition gives you several ways to work with the same local knowledge base: the desktop app, the command line and the HTTP API.</p>
    <figure class="page-image wide">
      <img src="{{ '/assets/img/platforms/windows/win-10-all-the-interfaces.png' | relative_url }}" alt="Holder on Windows showing the desktop app, command line interface and browser API documentation">
    </figure>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Signed Release Builds Are Coming Soon</h2>
    <p>Holder is more transparent than most software on your system. All of the source code is public, and the binaries are built using public GitHub workflows, so you can follow the process from source code to installer.</p>
    <p>You can also download the source code and build Holder yourself.</p>
    <p>However, getting those GitHub workflows to produce properly signed Windows installers has a few more hoops to jump through.</p>
    <p>It is good that Windows takes software security seriously, but the developer experience for distributing traditional Windows applications is fragmented. Apple has Developer ID and notarization, while Linux distributions such as Ubuntu have established package-signing infrastructure. Windows has Authenticode signing, SmartScreen reputation, optional Microsoft Store distribution and several packaging formats, and those pieces need to be fitted together into a GitHub release process.</p>
    <p>This is slightly ironic given that GitHub itself is owned by Microsoft, but anyway.</p>
    <p>The goal is for every Holder release for Windows to be Authenticode-signed using a certificate that chains to a trusted certificate authority. Over time, those signed releases should also build SmartScreen reputation. Holder can then also go through the Microsoft Store certification process.</p>
    <p>I haven't finished getting all of that working yet. For now, you can either use a development build or compile Holder yourself.</p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Staged Development Builds</h2>
    <p>Development builds are self-signed by the GitHub build workflow.</p>
    <p>Because the development signing certificate is not trusted by Windows, Microsoft Defender SmartScreen will treat the installer as an unrecognised application and display:</p>
    <p><strong>Windows protected your PC</strong></p>
    <figure class="page-image">
      <img src="{{ '/assets/img/platforms/windows/win-1-dont-run.png' | relative_url }}" alt="Windows SmartScreen showing the Windows protected your PC warning">
    </figure>
    <p>with a recommendation not to run the application.</p>
    <p>This does not mean that SmartScreen has detected malware in Holder. It means Windows does not trust the certificate used to sign these development builds, and the installer does not yet have normal publisher reputation.</p>
    <p>If you've downloaded Holder from the official Holder project and want to continue, click <strong>More info</strong>, check that you're running the file you intended to download, and then choose <strong>Run anyway</strong>.</p>
    <figure class="page-image">
      <img src="{{ '/assets/img/platforms/windows/win-2-run-anyway.png' | relative_url }}" alt="Windows SmartScreen showing the Run anyway button after expanding More info">
    </figure>
    <p>If you regularly test or develop Holder on Windows, you can install and trust the Holder development certificate on your development machine. Windows can then verify subsequent development builds using that certificate instead of treating Holder's development signing identity as unknown.</p>
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
