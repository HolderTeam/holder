---
layout: default
title: About Holder
permalink: /about/
---

<section class="page-panel">
  <p class="eyebrow">About</p>
  <h1>About Holder</h1>
  <p class="lead">Holder is a free, open-source, volunteer-built personal knowledge system.</p>
</section>

<section class="section">
  <div class="section-head">
    <h2>What is this?</h2>
    <p>Holder is a free, open-source, volunteer-built personal knowledge system.</p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>When will this be finished?</h2>
    <p>
      London is two thousand years old, look up, the skyline is full of cranes, look down they are digging new tube lines.
    </p>
    <p>
      With Holder, it is very new. There are rough edges. Things will change. Some platforms and features are further
      along than others. No point to pretend otherwise.
    </p>
    <p>But that's also why this is an interesting time to get involved.</p>
    <p>
      Holder is being built in the open, and I'd like other people to help shape what it becomes, whether that's
      through code, testing, packaging, documentation, design, ideas, bug reports, or simply trying it and figuring
      out what doesn't work.
    </p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Who is making this?</h2>
    <p>
      The project has been started by me, Zeth, a software engineer from England, but I am hoping others will join.
    </p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Why are you making this?</h2>
    <p>
      When I was young, anything important that I needed to remember, I would write on the back of my hand.
      If that wasn't enough space then I would write it on the back of an old envelope and use it as a bookmark
      for the current book that I carried with me. That was enough, it worked.
    </p>
    <p>
      But now a few decades later. I am underwater in an swirling chaotic ocean of information. I have emails,
      documents, forms, bills, ideas, research, bookmarks, AI conversations, notes, things I need to remember,
      things I need to write, and an endless stream of crap coming at me.
    </p>
    <p>
      How do I get on top of it? I found existing programs did not suit my needs, my ways of working. They were
      too closed, too commercial, too annoying. They were often worse than just a directory of text files.
    </p>
    <p>
      So I am building a tool for myself. And it could be useful for other people, so I am trying to make it an
      open source project. Then other people can contribute and we can collectively bring order to the chaos.
    </p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>How can I help?</h2>
    <p>
      The most useful way to start is to simply to get it working on your computer, use it seriously to try to
      organise yourself, and then think: what works, what doesn't, what's confusing, and what's missing.
    </p>
    <p>Contributions of all kinds are welcome. Development, testing, art, web design, writing.</p>
    <p>
      You don't need to be an expert on the whole project to contribute. If something interests you, pick a
      repository, have a look around, open an issue, join a discussion, or try building it.
    </p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>How Does Holder Fit Together?</h2>
    <p>
      Holder is an open ecosystem that hopefully can fit into your workflow. It is a framework that comes with a GUI,
      but also an HTTP API and a Command Line interface.
    </p>
    <p>It is made up in parts, each one does one thing well.</p>
    <p>
      The biggest two parts are the frontend and backend. If you have ever made a web application, you can get the idea.
    </p>
    <p>The backend has the core logic and the server, which in classic Unix terminology is called a daemon.</p>
    <p>The frontend has the UI desktop interface.</p>
    <p>
      In packaged managed setups like Ubuntu Linux, that is all we need. The service manager (Systemd or OpenRC) starts
      the daemon and the GUI frontend, the API and command line tool can just expect it to be there.
    </p>
    <p>
      But on Mac and Windows we need to handle this process ourselves. So a third program called the launcher checks
      the backend is up, starts it if needed, then hands over to the frontend.
    </p>
    <p>
      We also have a couple of more dev-ops repos that handle the workflows of getting the source code from repo to
      release.
    </p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Where is it?</h2>
    <p>
      Holder lives in the open on GitHub. That is where the source code, issues, discussions, contributor notes,
      and project repositories are collected.
    </p>
  </div>

  <div class="button-row">
    <a class="button primary" href="https://github.com/HolderTeam/">Visit HolderTeam on GitHub</a>
  </div>
</section>
