#!/usr/bin/env python3
"""Opt-in source runtime selector; importable by GUI test launchers."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shlex
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[4]
RECIPE = ROOT / 'staging/scripts/build-macos-accessible-gtk.sh'
GTK_PATCH = RECIPE.parent / 'patches/gtk-menu-label-role.patch'
PACKAGES = ('gtk4', 'libadwaita-1', 'gtksourceview-5', 'vte-2.91-gtk4',
            'libspelling-1', 'gio-2.0', 'gee-0.8', 'libsoup-3.0', 'json-glib-1.0')


def output(*args: str) -> str:
    return subprocess.check_output(args, text=True).strip()


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def identity() -> tuple[dict, dict]:
    # Include transitive pkg-config dependencies and actual dylib content, not just
    # GTK's version: Homebrew revisions can change ABI without changing that version.
    packages = set(PACKAGES)
    pending = list(PACKAGES)
    while pending:
        package = pending.pop()
        for flag in ('--print-requires', '--print-requires-private'):
            for line in output('pkg-config', flag, package).splitlines():
                name = line.split()[0]
                if name not in packages:
                    packages.add(name)
                    pending.append(name)
    dependencies = {}
    libraries = {}
    for package in sorted(packages):
        prefix = output('pkg-config', '--variable=prefix', package)
        libdir = output('pkg-config', '--variable=libdir', package)
        # Some .pc files (e.g. graphite2) omit prefix. An absent value must
        # never resolve to the launcher's current working directory.
        prefix = str(Path(prefix).resolve()) if prefix else ''
        libdir = str(Path(libdir).resolve()) if libdir else ''
        if libdir not in libraries:
            libraries[libdir] = {
                str(p.resolve()): digest(p.resolve())
                for p in sorted(Path(libdir).glob('*.dylib')) if p.is_file()
            } if libdir else {}
        dependencies[package] = {
            'version': output('pkg-config', '--modversion', package),
            'prefix': prefix,
            'libraries': libraries[libdir],
        }
    brew_env = dict(os.environ, HOMEBREW_NO_AUTO_UPDATE='1')
    formula = json.loads(output_with_env(['brew', 'info', '--json=v2', 'gtk4'], brew_env))
    gtk = formula['formulae'][0]
    version = dependencies['gtk4']['version']
    if version != gtk['versions']['stable']:
        raise RuntimeError('Installed GTK differs from Homebrew source metadata; '
                           'install matching GTK dependencies before preparing a runtime.')
    if gtk.get('patches'):
        raise RuntimeError('GTK source has Homebrew patches; review the shared recipe first.')
    config = {name: os.environ.get(name, '') for name in (
        'CC', 'CFLAGS', 'CPPFLAGS', 'LDFLAGS', 'SDKROOT',
        'MACOSX_DEPLOYMENT_TARGET', 'PKG_CONFIG_PATH', 'PKG_CONFIG_LIBDIR',
        'RUSTFLAGS', 'CARGO_BUILD_TARGET')}
    return {
        'format': 1, 'arch': platform.machine(), 'macos': platform.mac_ver()[0],
        'source': gtk['urls']['stable'], 'recipe': digest(RECIPE),
        'patches': {GTK_PATCH.name: digest(GTK_PATCH)},
        'selector': digest(Path(__file__)), 'dependencies': dependencies,
        'compiler': output('cc', '--version'), 'rust': output('rustc', '--version'),
        'sdk': output('xcrun', '--show-sdk-path'),
        'meson': output('meson', '--version'), 'configuration': config,
    }, formula


def runtime_env(runtime: Path, base: dict[str, str] | None = None) -> dict[str, str]:
    env = dict(os.environ if base is None else base)
    # Set this immediately before exec/Popen of the Mach-O executable. SIP can
    # strip DYLD_* when launching /bin/sh or /usr/bin/env in between.
    env['DYLD_LIBRARY_PATH'] = str(runtime / 'lib')
    env['GTK_A11Y'] = 'accesskit'
    env['HOLDER_ACCESSIBLE_RUNTIME'] = str(runtime)
    return env


PROBE = r'''
#include <gtk/gtk.h>
#include <adwaita.h>
#include <gtksourceview/gtksource.h>
#include <vte/vte.h>
#include <libspelling.h>
#include <mach-o/dyld.h>
#include <stdio.h>
int main(void) {
    printf("GTK %u.%u.%u; consumers: %lu %lu %lu %lu\n",
        gtk_get_major_version(), gtk_get_minor_version(), gtk_get_micro_version(),
        (unsigned long)adw_application_get_type(),
        (unsigned long)gtk_source_view_get_type(),
        (unsigned long)vte_terminal_get_type(),
        (unsigned long)spelling_checker_get_type());
    for (uint32_t i = 0; i < _dyld_image_count(); i++)
        puts(_dyld_get_image_name(i));
}
'''


def verify(runtime: Path) -> str:
    result = output_with_env([str(runtime.parent / 'runtime-probe')], runtime_env(runtime))
    images = result.splitlines()[1:]
    gtk = [Path(p).resolve() for p in images if Path(p).name.startswith('libgtk-4.')]
    accesskit = [Path(p).resolve() for p in images if 'libaccesskit' in Path(p).name]
    if gtk != [(runtime / 'lib/libgtk-4.1.dylib').resolve()]:
        raise RuntimeError(f'Expected one isolated GTK runtime, loaded: {gtk}')
    if not accesskit or any(not p.is_relative_to(runtime) for p in accesskit):
        raise RuntimeError(f'Isolated AccessKit not loaded: {accesskit}')
    return result


def output_with_env(args: list[str], env: dict[str, str]) -> str:
    return subprocess.check_output(args, env=env, text=True).strip()


def select_runtime() -> Path:
    if sys.platform != 'darwin':
        raise RuntimeError('The isolated GTK runtime is macOS-only.')
    expected, formula = identity()
    key = hashlib.sha256(json.dumps(expected, sort_keys=True).encode()).hexdigest()
    cache = Path(os.environ.get('HOLDER_ACCESS_CACHE',
                 str(ROOT / 'apps/desktop/gtk/build-access-runtime'))).resolve()
    cache.mkdir(parents=True, exist_ok=True)
    target = cache / key
    runtime = target / 'runtime'
    marker = target / 'identity.json'
    # A lock prevents two local launches from compiling into the same directory.
    import fcntl
    with (cache / '.lock').open('w') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        if marker.exists():
            record = json.loads(marker.read_text())
            if record['identity'] != expected or any(
                not (target / p).is_file() or digest(target / p) != sha
                for p, sha in record['artifacts'].items()
            ):
                raise RuntimeError(f'Runtime cache was modified or damaged: remove {target} and retry.')
            print(f'Reusing accessible GTK {expected["dependencies"]["gtk4"]["version"]}: {runtime}', file=sys.stderr)
        else:
            if target.exists():
                raise RuntimeError(f'Incomplete runtime: remove {target} and retry.')
            # Build at its permanent path: GTK and AccessKit contain absolute
            # install names, so renaming a temporary build would break loading.
            target.mkdir()
            formula_path = target / 'formula.json'
            formula_path.write_text(json.dumps(formula))
            env = os.environ.copy()
            env['HOLDER_GTK_FORMULA_JSON'] = str(formula_path)
            print(f'Preparing accessible GTK runtime: {runtime}', file=sys.stderr)
            subprocess.run(['bash', str(RECIPE), str(target)], env=env, check=True)
            probe_source = target / 'runtime-probe.c'
            probe_source.write_text(PROBE)
            flags = shlex.split(output('pkg-config', '--cflags', '--libs', *PACKAGES))
            subprocess.run(['cc', str(probe_source), '-o', str(target / 'runtime-probe'), *flags], check=True)
            verify(runtime)
            # Reject dependency changes during a long first-use build.
            if identity()[0] != expected:
                raise RuntimeError('Dependencies changed during preparation; retry with stable dependencies.')
            artifacts = {str(p.relative_to(target)): digest(p) for p in runtime.rglob('*') if p.is_file()}
            artifacts['runtime-probe'] = digest(target / 'runtime-probe')
            marker.write_text(json.dumps({'identity': expected, 'artifacts': artifacts}, indent=2) + '\n')
        print(verify(runtime).splitlines()[0], file=sys.stderr)
    return runtime


def launch_environment(base: dict[str, str] | None = None) -> dict[str, str]:
    """For test drivers: pass this directly to Popen([native_app], env=...)."""
    if sys.platform == 'darwin':
        return runtime_env(select_runtime(), base)
    if sys.platform.startswith('linux'):
        env = dict(os.environ if base is None else base)
        env.update(GTK_A11Y='atspi', NO_AT_BRIDGE='0')
        return env
    raise RuntimeError('Accessible mode supports macOS and Linux.')


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=('prepare', 'inspect', 'check-menus', 'run'))
    parser.add_argument('command', nargs=argparse.REMAINDER)
    args = parser.parse_args()
    if args.mode == 'run':
        if not args.command:
            parser.error('run requires a native executable')
        env = launch_environment()
        executable = Path(args.command[0]).resolve()
        if sys.platform == 'darwin' and executable.read_bytes()[:4] not in (
            b'\xcf\xfa\xed\xfe', b'\xfe\xed\xfa\xcf', b'\xca\xfe\xba\xbe'):
            raise RuntimeError('Launch the Mach-O executable directly, without a shell or open -a.')
        os.execve(executable, [str(executable), *args.command[1:]], env)
    else:
        runtime = select_runtime()
        if args.mode == 'check-menus':
            source = ROOT / 'apps/desktop/gtk/tests/macos_menu_accessibility_test.m'
            executable = runtime.parent / 'menu-accessibility-test'
            flags = shlex.split(output('pkg-config', '--cflags', '--libs', 'gtk4'))
            subprocess.run(['cc', str(source), '-o', str(executable),
                            '-framework', 'Cocoa', *flags], check=True)
            subprocess.run([str(executable)], env=runtime_env(runtime), check=True, timeout=30)
        else:
            print(runtime if args.mode == 'prepare' else verify(runtime))


if __name__ == '__main__':
    try:
        main()
    except (RuntimeError, subprocess.CalledProcessError, OSError) as error:
        print(f'Accessible runtime: {error}', file=sys.stderr)
        sys.exit(1)
