# Python

Holder's Python libraries. Choose by how your program reaches Holder:

- **[`holder/`](holder/) (`holder-python` on PyPI, `import holder`)** talks to a running Holder
  through the daemon's HTTP API, as the desktop app does. Use it to script or automate the Holder
  you use day to day; it needs the daemon running.
- **[`kit/`](kit/), Holder Kit (`holder-kit` on PyPI, `import holderkit`)** embeds Holder's
  engine, core, in your Python process and works offline with no daemon, for example for analysis
  or batch work on a Holder home.

Each library has its own README with setup and development instructions.
