#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Re-vendor the Yuri's Revenge port's pinned trees from a redalert2-decomp checkout at a commit:
#
#   tools/update_yr_vendor.sh <redalert2-decomp checkout> <commit>
#
# Writes vendor/ra2-decomp (yr/ra2-decomp of that checkout: gibbo101/ra2-decomp at the commit in its PIN) and
# vendor/tibsun (yr/tibsun: OpenTS-Developers/TibSun's headers at the commit in its PIN), copied unchanged by git
# archive, with a PIN in each: the upstream commit, the redalert2-decomp commit they came from, and the SHA-256 of
# every vendored file (tools/check_tree.py checks it). The checkout is only read.
set -euo pipefail
[[ $# -ge 2 ]] || { sed -n '3,6p' "$0" | sed 's/^# \{0,1\}//' >&2; exit 64; }
decomp=$(cd "$1" && pwd)
commit=$(git -C "$decomp" rev-parse --verify "$2^{commit}")
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/.." && pwd)
work=$(mktemp -d "${TMPDIR:-/tmp}/yr-revendor.XXXXXX")
trap '/bin/rm -rf "$work"' EXIT
git -C "$decomp" archive "$commit" yr/ra2-decomp yr/tibsun yr/README.md | tar -x -C "$work"
upstream() {   # the upstream commit of a copy, from yr/README.md's table row
  grep "^| \`$1/\`" "$work/yr/README.md" | grep -oE '[0-9a-f]{40}' | head -1
}
for name in ra2-decomp tibsun; do
  vendor="$root/vendor/$name"
  /bin/rm -rf "$vendor"
  /bin/cp -R "$work/yr/$name" "$vendor"
  up=$(upstream "$name")
  [[ -n "$up" ]] || { echo "update_yr_vendor: no upstream commit for $name in yr/README.md" >&2; exit 1; }
  {
    echo "# vendor/$name: yr/$name of redalert2-decomp at the commit below, copied unchanged by"
    echo "# tools/update_yr_vendor.sh. Never edit these files: port changes go in yr/fixups.json and yr/glue."
    echo "# tools/check_tree.py checks this list."
    echo "upstream $up"
    echo "redalert2-decomp $commit"
    (cd "$vendor" && find . -type f ! -name PIN | sed 's#^\./##' | LC_ALL=C sort | while read -r f; do
       echo "sha256 $(shasum -a 256 "$f" | cut -c1-64) $f"
     done)
  } > "$work/PIN"
  /bin/mv -f "$work/PIN" "$vendor/PIN"
  echo "update_yr_vendor: vendor/$name at ${up:0:10} ($(grep -c '^sha256 ' "$vendor/PIN") files)"
done
