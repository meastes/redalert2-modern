#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Re-vendor the rebuild tree from a redalert2-decomp checkout at a commit:
#
#   tools/update_rebuild.sh <redalert2-decomp checkout> <commit> [<public snapshot commit>]
#
# Writes vendor/ra2-rebuild/: src/ and include/ (rebuild/src, rebuild/include), LICENSE.md (rebuild/LICENSE.md),
# adopted.csv (rebuild/manifest/adopted.csv), all copied unchanged from the commit by git archive; symbols.tsv
# derived from rebuild/manifest/catalog by tools/make_symbols.py (names and addresses only); and PIN: the commit,
# the public snapshot commit when given, and the SHA-256 of every vendored file (tools/check_tree.py checks it).
# The checkout is only read. Its own Python (with its tools/original_target) reads the catalog: RA2_DECOMP_PYTHON,
# else python3.
#
# After a re-vendor: ra2/fixups.json rows that no longer match make the configure stop ("fixups that match
# nothing"); fix them, build, and run the played checks (README "Checks") before committing the new PIN.
set -euo pipefail
[[ $# -ge 2 ]] || { sed -n '3,6p' "$0" | sed 's/^# \{0,1\}//' >&2; exit 64; }
decomp=$(cd "$1" && pwd)
commit=$(git -C "$decomp" rev-parse --verify "$2^{commit}")
snapshot="${3:-unknown}"
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/.." && pwd)
vendor="$root/vendor/ra2-rebuild"
work=$(mktemp -d "${TMPDIR:-/tmp}/ra2-revendor.XXXXXX")
trap '/bin/rm -rf "$work"' EXIT

git -C "$decomp" archive "$commit" rebuild/src rebuild/include rebuild/LICENSE.md rebuild/manifest/adopted.csv \
    rebuild/manifest/catalog | tar -x -C "$work"
/bin/rm -rf "$vendor/src" "$vendor/include"
mkdir -p "$vendor"
/bin/cp -R "$work/rebuild/src" "$work/rebuild/include" "$vendor/"
/bin/cp -f "$work/rebuild/LICENSE.md" "$vendor/LICENSE.md"
/bin/cp -f "$work/rebuild/manifest/adopted.csv" "$vendor/adopted.csv"
"${RA2_DECOMP_PYTHON:-python3}" -B "$here/make_symbols.py" --decomp "$decomp" --catalog "$work/rebuild/manifest/catalog" \
    --out "$vendor/symbols.tsv"

{
  echo "# vendor/ra2-rebuild: rebuild/ of redalert2-decomp at the commit below, copied unchanged by"
  echo "# tools/update_rebuild.sh: src/ and include/, LICENSE.md (rebuild/LICENSE.md), adopted.csv"
  echo "# (rebuild/manifest/adopted.csv); symbols.tsv derived from rebuild/manifest/catalog (tools/make_symbols.py)."
  echo "# Never edit these files: port changes go in ra2/fixups.json and ra2/glue. tools/check_tree.py checks this list."
  echo "commit $commit"
  echo "public-snapshot $snapshot"
  (cd "$vendor" && find . -type f ! -name PIN | sed 's#^\./##' | LC_ALL=C sort | while read -r f; do
     echo "sha256 $(shasum -a 256 "$f" | cut -c1-64) $f"
   done)
} > "$work/PIN"
/bin/mv -f "$work/PIN" "$vendor/PIN"
echo "update_rebuild: vendor/ra2-rebuild at ${commit:0:10} ($(grep -c '^sha256 ' "$vendor/PIN") files)"
