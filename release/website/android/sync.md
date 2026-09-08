---
layout: default
title: Android Project Sync
permalink: /android/sync/
---

<section class="page-panel">
  <p class="eyebrow">Android</p>
  <h1>Project Sync</h1>
  <p class="lead">Keep a project's cards up to date across your phone, your laptop, and anyone else you're working with — with real history, not just a snapshot.</p>
</section>

<section class="section">
  <div class="section-head">
    <h2>What it actually is</h2>
    <p>
      Under the hood, every Holder project is a Git repository. Project Sync pushes and pulls that repository the
      same way any other Git workflow would — it's not a proprietary sync protocol, and it's not tied to Holder's
      own servers. There's no Holder backend involved at all: your device talks directly to whichever Git host is
      holding the repository.
    </p>
    <p>
      That means real history. Every change is a real commit. Nothing is capped in size the way <a href="{{ '/android/backups/' | relative_url }}">Backup</a>
      is, and restoring an older version of a card is a normal Git operation, not something Holder has to build a
      special mechanism for.
    </p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>The paved road: GitHub</h2>
    <p>
      Most people don't want to think about Git hosts, access tokens, or SSH keys. Settings → Sync has a "Connect"
      button that signs you into GitHub and installs a small, purpose-built GitHub App (Holder Project Setup) —
      Device Flow is never used, and the app never sees your GitHub password. Once that's done, creating a new
      project with the default options is enough: Holder creates a private repository for it, registers this
      device's own key against it, and pushes and pulls in the background from then on.
    </p>
    <p>
      You never need a GitHub account to just use Holder locally — this is entirely optional, and a project only
      ever syncs if you choose to connect it.
    </p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>The choices when you create a project</h2>
    <p>New Project has a "Custom options" section, collapsed by default, for three independent choices:</p>
    <p>
      <strong>Card body — Encrypted or Plain text.</strong> Encrypted is the default: every card is encrypted with a
      key that lives only on your own devices, never on the Git host. GitHub (or wherever the repository lives)
      only ever sees ciphertext. Plain text stores cards as ordinary Markdown files instead — useful if you want to
      open or edit them with other tools, but be careful what you put in them, since anyone with access to the
      repository can read them directly.
    </p>
    <p>
      <strong>Project Sync — Synced or Device only.</strong> Synced is the default. Device only means exactly what
      it says: the project never leaves this device at all, which also means there's nothing to fall back on if
      the device is lost, damaged, or reset (see <a href="{{ '/android/backups/' | relative_url }}">Backup</a> for
      the one weak safety net that still applies even then).
    </p>
    <p>
      <strong>Visibility — Private or Public.</strong> Only relevant once you've chosen Synced. Private is the
      default and the sane choice for almost everyone. Public means the repository — and therefore anyone's cards
      in it, plain text or not — can be browsed by anyone on the web. Even an encrypted project's <em>name</em>
      is still visible on a public repository, since GitHub itself needs it to be, regardless of what's encrypted
      inside.
    </p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Using your own Git server instead</h2>
    <p>
      GitHub is the paved road, not the only road. Every project's own Project Sync screen has a plain "Remote
      repository" field you can point at any Git URL — a self-hosted server, GitLab, Bitbucket, whatever you
      already use. A "See device key" toggle reveals this device's own SSH key for that specific project (each
      project gets its own key, since most Git hosts, GitHub included, won't let the same key act as a deploy key
      on two different repositories), ready to paste in wherever your Git host wants a deploy key.
    </p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Moving a project to a new device</h2>
    <p>
      If a device already has the project's own history somewhere reachable, connecting it to the same remote and
      pulling is enough. Settings → Import Project (or the same option on an empty project list) exists for a
      slightly different case: you have a recovery key — exported earlier from the Project Sync screen's
      Encryption recovery section — and want to open that project on a device that doesn't have it yet. Paste the
      key and its PIN, and Holder either recreates the project fresh or re-registers this device's own key,
      whichever the situation calls for.
    </p>
  </div>
</section>

<section class="section">
  <div class="section-head">
    <h2>Not the same thing as Backup or Storage</h2>
    <p>
      Three different Settings pages sound similar and aren't. <strong>Sync</strong> (this page) is full project
      history, ongoing, for projects you've deliberately connected to a remote. <a href="{{ '/android/backups/' | relative_url }}">Backup</a>
      is a small, automatic, no-setup-required safety net covering only your most recent cards, with no history at
      all. <a href="{{ '/android/storage/' | relative_url }}">Storage</a> is neither of those — it's where photos
      and files attached to cards live, completely separate from the cards' own text content.
    </p>
  </div>
</section>
