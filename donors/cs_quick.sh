#!/usr/bin/env bash
# Kompilasi satu objek kernel di codespace (loop cepat, tanpa root).
#
#   donors/cs_quick.sh                                     # debugstat.o
#   donors/cs_quick.sh drivers/foo/bar.o
#   CS=<nama-codespace> donors/cs_quick.sh ...             # codespace lain
#
# Kerja: file yang berubah (tracked + untracked) di dalam pohon kernel dikirim
# lewat tar/ssh ke worktree bersih di codespace, lalu target itu di-`make`.
# Incremental ~10 s; dari nol ~30 s karena `merge_config.sh` + `olddefconfig`
# ikut jalan.
#
# Prasyarat di codespace (semua tanpa sudo, lihat docs/BUILD-ENV.md):
#   $HOME/tc     toolchain ARM 13.2 rel1 (aarch64-none-linux-gnu-)
#   $HOME/bc     bc dari .deb Ubuntu noble
#   $HOME/flex   flex hasil build sendiri
#   $REMOTE      git worktree bersih pada commit yang mau diuji
set -euo pipefail

CS=${CS:-reimagined-palm-tree-g49xvp9vxx652ww5q}
TOKEN_FILE=${TOKEN_FILE:-/tmp/afrizal_pat.txt}
REMOTE=${REMOTE:-/workspaces/b2}
K=6.18.52-VXTux-UMS512
TARGET=${1:-drivers/vxtux/soc/debugstat/sprd_debugstat.o}

if [ -z "${GH_TOKEN:-}" ] && [ -r "$TOKEN_FILE" ]; then
	GH_TOKEN=$(cat "$TOKEN_FILE")
	export GH_TOKEN
fi

RENV="export PATH=\$HOME/bc/bin:\$HOME/flex/bin:\$HOME/tc/bin:\$PATH"

# 1. Kirim hanya yang berubah, satu round-trip.
mapfile -t files < <({ git diff --name-only HEAD -- "$K"; git ls-files --others --exclude-standard -- "$K"; } | sort -u)
if [ "${#files[@]}" -gt 0 ]; then
	printf '%s\n' "${files[@]}" | tar czf - -T - |
		gh cs ssh -c "$CS" -- "mkdir -p $REMOTE && tar xzf - -C $REMOTE"
	echo "SYNC ${#files[@]} file"
else
	echo "SYNC 0 (tidak ada perubahan)"
fi

# 2. Kompilasi target. Log penuh disimpan di $REMOTE/last-cc.log.
gh cs ssh -c "$CS" -- "$RENV; cd $REMOTE/$K || exit 3
S=\$(date +%s)
make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- -j\$(nproc) '$TARGET' > $REMOTE/last-cc.log 2>&1
rc=\$?
echo \"rc=\$rc waktu=\$(( \$(date +%s) - S ))s\"
grep -E 'error:|Error [0-9]|warning:' $REMOTE/last-cc.log | grep -v modpost.h | head -40 || true
exit \$rc"
