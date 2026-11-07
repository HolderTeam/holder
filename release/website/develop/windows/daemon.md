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
    <p>Follow the <a href="{{ '/develop/windows/' | relative_url }}">Windows developer setup</a> to install Git and get a local checkout of Holder with its submodule.</p>
    <p>Install <a href="https://visualstudio.microsoft.com/vs/community/">Visual Studio Community</a> (2022 or later) with the <strong>Desktop development with C++</strong> workload. Include the CMake tools, Windows SDK, and vcpkg. Both routes below use these tools.</p>
    <p>Open the Windows Start menu, search for <strong>x64 Native Tools Command Prompt for VS</strong>, and open it. The window should say <strong>Environment initialized for: 'x64'</strong>. If your prompt starts with <code>PS</code>, switch to the Native Tools Command Prompt window you opened from Start.</p>
    <p>The first configuration can take a while. vcpkg downloads and builds the libraries Daemon depends on.</p>
    <p>Choose either route to build and test Daemon.</p>
    <ul>
      <li><a href="#vs-code">Visual Studio Code</a></li>
      <li><a href="#command-line">Command line</a></li>
    </ul>
  </div>
</section>

<section class="section" id="download-daemon">
  <div class="section-head">
    <h2>Daemon and Core</h2>
    <p>The Daemon Windows debug build compiles Core from the neighbouring <code>core</code> folder in the same checkout, so changes you make in Core are used the next time you build Daemon. To build Core on its own and run its tests, follow the <a href="{{ '/develop/windows/core/' | relative_url }}">Core guide</a>.</p>
  </div>
</section>

<section class="section" id="build-daemon">
  <div class="section-head">
    <h2 id="vs-code">Build and test in Visual Studio Code</h2>
    <p>Install <a href="https://code.visualstudio.com/">Visual Studio Code</a> and Microsoft's <a href="https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools">CMake Tools</a> extension. When installing VS Code, enable <strong>Add to PATH</strong> so the Command Prompt can open it.</p>
  </div>
  <ol class="steps">
    <li>
      Close any open VS Code windows so it can pick up the build environment when it starts. If you just installed VS Code, reopen the Native Tools Command Prompt so it can find VS Code. Run the command below in that prompt. If you cloned Holder somewhere else, use the path to your <code>daemon</code> folder.
      <pre><code>code "%USERPROFILE%\projects\holderteam\holder\daemon"</code></pre>
    </li>
    <li>Open the Command Palette with <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>P</kbd>. Choose <strong>CMake: Select Configure Preset</strong>, select <strong>Windows vcpkg Debug</strong> (<code>windows-vcpkg-debug</code>), then choose <strong>CMake: Configure</strong> and wait for it to finish.</li>
    <li>Choose <strong>CMake: Select Build Preset</strong> and select <code>windows-vcpkg-debug</code>. Choose <strong>CMake: Build</strong> to build <code>holderd.exe</code> and <code>holderctl.exe</code>.</li>
    <li>To build the tests, select <strong>Windows vcpkg Debug Tests</strong> (<code>windows-vcpkg-tests-debug</code>) as the configure preset. Configure it, select <code>windows-vcpkg-tests-debug</code> as the build preset, and build again.</li>
    <li>Choose <strong>CMake: Select Test Preset</strong> and select <code>windows-vcpkg-tests-debug</code>. Choose <strong>CMake: Run Tests</strong> to run the tests.</li>
  </ol>
</section>

<section class="section" id="command-line">
  <div class="section-head">
    <h2>Build and test from the command line</h2>
    <p>Run these commands one at a time in the Native Tools Command Prompt. If you cloned Holder somewhere else, replace the path in the first command with your <code>daemon</code> folder.</p>
  </div>
  <ol class="steps">
    <li>
      Change to the <code>daemon</code> folder.
      <pre><code>cd /d "%USERPROFILE%\projects\holderteam\holder\daemon"</code></pre>
    </li>
    <li>
      Configure the debug build. CMake uses the saved preset and calls vcpkg to install the dependencies.
      <pre><code>cmake --preset windows-vcpkg-debug</code></pre>
    </li>
    <li>
      Build the backend and command-line tool.
      <pre><code>cmake --build --preset windows-vcpkg-debug --parallel</code></pre>
    </li>
    <li>
      Configure a debug build with tests enabled.
      <pre><code>cmake --preset windows-vcpkg-tests-debug</code></pre>
    </li>
    <li>
      Build the programs and their tests.
      <pre><code>cmake --build --preset windows-vcpkg-tests-debug --parallel</code></pre>
    </li>
    <li>
      Run the tests. CTest shows the output of failed tests and reports an error if it finds no tests.
      <pre><code>ctest --preset windows-vcpkg-tests-debug --no-tests=error</code></pre>
    </li>
  </ol>
</section>

<section class="section">
  <div class="section-head">
    <h2>Check the result</h2>
    <p>The debug build puts <code>holderd.exe</code> and <code>holderctl.exe</code> in <code>daemon\out\build\windows-vcpkg-debug</code>. Confirm that tests ran without failures. A few tests are deliberately skipped on Windows because they check behaviour specific to other platforms.</p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>If a build seems stuck</h2>
    <p>The first vcpkg build can be long and quiet. Open Task Manager and check for active compiler processes before deciding that it has stopped.</p>
    <p>If a step fails, copy the error from the VS Code Output panel or the Command Prompt and <a href="{{ '/develop/windows/' | relative_url }}#getting-help">ask for help</a>.</p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Continue</h2>
    <p>You can now <a href="{{ '/develop/windows/' | relative_url }}#contributing">share your first change</a>. The <a href="https://github.com/HolderTeam/holder/tree/main/daemon#readme">Daemon README</a> has more development details.</p>
    <p>If you also want to work on the user interface, <a href="{{ '/develop/windows/desktop/' | relative_url }}">build the Holder desktop app</a>. That guide also shows how to start Daemon and Desktop together.</p>
    <p>If a Daemon change also needs a Core change, make it in <code>holder\core</code>, run Core's tests too, and include both in the same pull request.</p>
  </div>
</section>
