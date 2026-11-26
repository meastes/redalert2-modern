# SPDX-License-Identifier: GPL-3.0-or-later
"""Host-neutral spellings for the build's generators and link tools (lane P42).

The generators write CMake lists (gen/units.cmake, coff_units.cmake) and #line directives naming files of the
build; on a Windows host those paths come back from pathlib with backslashes, which CMake reads as escapes
("Invalid character escape '\\w'") and C reads as string escapes. Both get forward slashes, which CMake, clang
and Windows itself all accept.

The link tools run the host's nm and c++filt. A Mac has both; a Windows host with llvm-mingw has llvm-nm and
llvm-cxxfilt (in RA2_LLVM_MINGW/bin); RA2_NM and RA2_CXXFILT name others.
"""
import os
import shutil

# Characters a quoted CMake argument would still interpret (or a list would split on): refused, not guessed at.
CMAKE_UNSAFE = '"$;'


def slashed(path):
    """PATH (a str or Path, of either host's spelling) with forward slashes."""
    return str(path).replace('\\', '/')


def cmake_path(path):
    text = slashed(path)
    bad = sorted(set(c for c in text if c in CMAKE_UNSAFE))
    if bad:
        raise SystemExit('host_tools: %r holds %s, which a CMake list cannot carry: build from a folder without them'
                         % (text, ' '.join(bad)))
    return text


def cmake_list(var, paths):
    """set(VAR "<path>" ...) with each path in forward slashes, one per line."""
    return 'set(%s\n%s)\n' % (var, ''.join('  "%s"\n' % cmake_path(p) for p in paths))


def line_directive(path):
    """#line 1 "<path>" for a generated copy of PATH."""
    return '#line 1 "%s"\n' % slashed(path).replace('"', '\\"')


TOOLS = {'nm': ('RA2_NM', 'llvm-nm'), 'c++filt': ('RA2_CXXFILT', 'llvm-cxxfilt')}


def tool(name):
    """The host's NAME ('nm' or 'c++filt'): $RA2_NM / $RA2_CXXFILT, else NAME on PATH, else llvm-mingw's llvm-<name>."""
    env, alt = TOOLS[name]
    if os.environ.get(env):
        return os.environ[env]
    if shutil.which(name):
        return name
    found = shutil.which(alt)
    if found:
        return found
    mingw = os.environ.get('RA2_LLVM_MINGW', '')
    if mingw:
        found = shutil.which(alt, path=os.path.join(mingw, 'bin'))
        if found:
            return found
    return name
