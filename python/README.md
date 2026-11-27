# Python

Install `holder-python` and use `import holder` for scripting, offline work,
and analysis.

- Use [`Holder()`](holder/README.md#get-started) to work with a running Holder
  installation.
- Use [`open_workspace(path)`](holder/README.md#shared-card-operations) to
  work in a private directory with the embedded backend.
- Use [`open_session(fallback_workspace=path)`](holder/README.md#run-with-or-without-a-local-holder-installation)
  when a script can use either backend.

For Kit examples, see [From Holder Kit to `holder`](holder/docs/from-holder-kit.md).
The [Kit source and development docs](kit/) cover the standalone package and
the extension used to build native Holder wheels.
