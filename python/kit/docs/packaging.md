# Packaging Holder Kit

The Python distribution is `holder-kit`; its import package is `holderkit`.
The version in `pyproject.toml` is independent of libholder and the Holder
Framework. The base package has no data-science dependencies; `pandas` and
`graph` are optional extras.

See [Building](building.md) for prerequisites and core selection, and
[Development](development.md) for checks to run before packaging.

## Wheels

The build backend is scikit-build-core, which invokes CMake to compile
`holderkit._native`. Wheels target the selected CPython and platform; they are
not stable-ABI wheels.

```sh
./make.sh wheel Release           # build with ../../core from source
./make.sh --sdk wheel Release     # build with the core SDK in HOLDER_CORE_SDK
```

Both commands write wheels to `out/make/wheels`. For a manual SDK build, use a
prepared Release SDK and a matching build configuration:

```sh
export HOLDER_CORE_SDK=/path/to/release-sdk
export SKBUILD_CMAKE_BUILD_TYPE=Release
python -m pip install build
python -m build --wheel --outdir dist
```

The package includes the matching `_schema.sql` and `py.typed`. SDK builds also
install the core manifest as `holderkit/_core_build.json`, recording the exact
core commit, version, platform, configuration and compiler. Source builds record
provenance in `out/make/core-source.json`; they do not currently install that
manifest into the Python package.

On Windows, SDK builds install runtime DLLs under `holderkit/.libs` and their
license notices under `holderkit/_licenses`. Unrepaired Linux and macOS SDK
wheels depend on the platform's native libraries. The canonical Linux SDK uses
the Ubuntu 24.04 dependency ABI. The release workflow bundles these dependencies
and checks the resulting wheel's platform compatibility as described below.

Validate a wheel in a fresh virtualenv outside the source checkout: import
`holderkit`, create temporary data, exercise the card lifecycle, and verify the
schema and typing resources. Test base use before installing optional adapters.
The Linux job in [Kit / CI](../../../.github/workflows/kit-ci.yml) provides the
installed SDK-wheel smoke test, including `_core_build.json` validation.

### Release wheels

`Release / Kit to PyPI`
([`release-kit-pypi.yml`](../../../.github/workflows/release-kit-pypi.yml))
builds wheels for CPython 3.10–3.14 and a source archive. Supported wheel platforms are:

| Platform | Architecture | Minimum runtime |
| --- | --- | --- |
| Linux | x86_64 | glibc 2.39 (Ubuntu 24.04 or compatible) |
| Windows | x86_64 | Windows 10/11 |
| macOS | ARM64 | macOS 15 |

Release builds use `Release` core SDKs that `Core / SDK` builds in the same run
from `core/` in the same commit. They are cached by `core/`'s tree, so a release
usually reuses the SDKs its Daemon CI run built.

Before building wheels and the source archive, the release workflow rewrites
relative README links in its checkout to absolute URLs at the release commit.
Documentation links use GitHub pages; images use raw GitHub image URLs. The
repository README keeps its relative links. Distribution validation checks
that every wheel description, the source README and source metadata contain
the same resolved description.

The wheels embed static core and include its native runtime dependencies, the
matching schema, type information and supplier notices. Installed wheels need
no core checkout, compiler or separately installed `libholder0`.

Linux repair bundles the dependency closure with private library names and uses
auditwheel to verify the `manylinux_2_39_x86_64` tag. GLib is included explicitly
because minimal Python environments may lack it. macOS uses delocate; Windows
uses delvewheel and removes the unmodified DLL copies before repair. Supplier
notices are carried in the distribution's licence metadata.

Each wheel is installed in a fresh virtualenv and tested before optional
adapters are installed, then runs the installed integration tests. Linux also
runs in clean Debian Python containers. The source archive is built and
installed with the same Release SDK. The final distribution check requires all
15 wheels and the source archive, checks each wheel's core against the SDKs'
`core-selection`, validates metadata, and records the core and the file sizes
and SHA256 hashes in the `kit-release-manifest` artifact.

To inspect a candidate from the `kit-release-distributions` artifact:

```sh
python -m venv .venv
source .venv/bin/activate  # Windows: .venv\Scripts\activate
python -m pip install /path/to/holder_kit-*.whl
python -c "import holderkit; print(holderkit.__file__)"
```

Select the wheel matching the interpreter and platform. Older glibc, musl
Linux, other architectures and other Python implementations require an
explicit [native source build](building.md).

The manual workflow's `destination` defaults to `none`, which builds and tests
candidates. `pypi` or `testpypi` publishes only after all distribution checks
pass, using the corresponding GitHub environment and Trusted Publishing.
Holder Kit is released with the rest of Holder, at the same version: publishing
must run from the release's `holder-v<version>` tag, and requires the version in
`pyproject.toml`, the repository's `VERSION` and core's version to equal it. Pull requests build candidates without
publishing.

## Ubuntu packages

`python3-holder-kit` installs for the distribution's Python in
`/usr/lib/python3/dist-packages`. It builds against that Ubuntu series'
`libholder-dev` and uses the shared `libholder0` at runtime. pandas and NetworkX
are suggested packages, not required runtime dependencies.

The Debian packaging lives in [`packaging/linux/debian`](../packaging/linux/debian/).
It uses distro CMake and Python directly, without fetching pip build dependencies.

### System core build

With the Holder PPA enabled, install `libholder-dev`, Python development headers,
CMake, Ninja and compilers, then configure shared-library mode:

```sh
cmake -S . -B build/system -G Ninja -DHOLDER_USE_SYSTEM_CORE=ON
cmake --build build/system
```

System mode requires core's `Holder::Shared` target and rejects simultaneous SDK
or source selections. It installs the matching schema and `_core_build.json`,
recording the core version, Debian package revision and schema digest. It does
not copy libholder into the Python package.

For a Debian binary build in a disposable checkout with the build dependencies
from `packaging/linux/debian/control` installed:

```sh
cp -a packaging/linux/debian debian
dpkg-buildpackage -b -us -uc
```

### Package validation

Kit / CI ([`kit-ci.yml`](../../../.github/workflows/kit-ci.yml)) builds the
package for Noble (Ubuntu 24.04) and Resolute (Ubuntu 26.04) separately, as
Launchpad does: from `python/kit/` copied out on its own, against libholder
packages built in the same job from `core/` in the same commit. It checks that
the package requires `libholder0` of the same upstream version (Kit installs
that core's schema and hands it to `libholder0` at runtime). It then installs
the packages in a fresh container without development packages or a compiler,
tests ordinary Python use with
[`scripts/ubuntu-package-ci.sh`](../scripts/ubuntu-package-ci.sh), then
installs optional adapters and runs integration tests.

### Launchpad source packages

`holder-kit` is one of the source packages that `Release / Launchpad`
([`release-launchpad.yml`](../../../.github/workflows/release-launchpad.yml))
uploads to `ppa:holderteam/holder`, with `holder-core` and `holder-daemon`. The
release tag uploads all three. A manual run with `packages=kit` and a new
`package_revision` uploads only Kit, for a packaging-only fix. Kit's version
must equal core's: its source package builds against `libholder-dev` of that
version, so on Launchpad it waits for the core packages from the same upload.
The upstream tarball is `python/kit/` as committed. Pull requests that change
the packaging build the source package unsigned without uploading.
