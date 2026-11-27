# SPDX-License-Identifier: GPL-3.0-or-later
"""Host-neutral spellings for the build's generators and link tools (lane P42).

The generators write CMake lists (gen/units.cmake, coff_units.cmake) and #line directives naming files of the
build; on a Windows host those paths come back from pathlib with backslashes, which CMake reads as escapes
("Invalid character escape '\\w'") and C reads as string escapes. Both get forward slashes, which CMake, clang
and Windows itself all accept.

The link tools run the host's nm and c++filt. A Mac has both; a Windows host with llvm-mingw has llvm-nm and
llvm-cxxfilt (in RA2_LLVM_MINGW/bin); RA2_NM and RA2_CXXFILT name others.

A tool run over many objects takes them in batches (batches()): a Windows command line holds at most 32,767
characters (CreateProcess; WinError 206 past it), and 400 build paths are more than that (lane P51: codex1's PC).
"""
import os
import shutil
import subprocess

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


# CreateProcess's limit is 32,767 characters with the terminating NUL; the margin covers the executable's path
# as Windows resolves it.
COMMAND_LIMIT = 32000


def batches(cmd, paths, count, limit=COMMAND_LIMIT):
    """PATHS in order, in runs of at most COUNT whose command line (CMD + the run, quoted as subprocess quotes it
    on Windows) stays within LIMIT characters; a path too long for any run goes alone. With short paths the
    runs are those of range(0, len(paths), COUNT), so a Mac build's output is unchanged."""
    base = len(subprocess.list2cmdline([str(c) for c in cmd]))
    run, size = [], base
    for path in paths:
        width = len(subprocess.list2cmdline([str(path)])) + 1
        if run and (len(run) == count or size + width > limit):
            yield run
            run, size = [], base
        run.append(path)
        size += width
    if run:
        yield run
