#!/bin/bash
# build_dtb41.sh -- bangun ums512-1h10-vxtux.dtb dari DTS saat ini tanpa
# membangun seluruh kernel.  Hanya butuh cpp (gcc) + dtc.
#
# Kenapa dipisah: DTB yang dipakai v39/v40 berasal dari artefak CI commit lama,
# sehingga perubahan sc2730.dtsi (nama rail + regulator-compatible) tidak
# pernah ikut ke image.  DTB harus ikut berubah bersama DTS.
set -eu
cd "$(dirname "$0")/.."
SRC=6.18.52-VXTux-UMS512
OUT=out/dtb_artix41
DTC=$SRC/scripts/dtc/dtc.exe

mkdir -p "$OUT"

# 1. dtc: pakai yang sudah ada, kalau belum bangun dari scripts/dtc.
if [ ! -x "$DTC" ]; then
  echo "[dtb] membangun dtc dari $SRC/scripts/dtc ..."
  ( cd "$SRC/scripts/dtc" \
    && flex -o dtc-lexer.lex.c dtc-lexer.l \
    && bison -o dtc-parser.tab.c -d dtc-parser.y \
    && gcc -O2 -w -DNO_YAML -I. -Ilibfdt -o dtc.exe \
         dtc.c flattree.c fstree.c livetree.c treesource.c data.c srcpos.c \
         checks.c util.c dtc-lexer.lex.c dtc-parser.tab.c \
         libfdt/fdt.c libfdt/fdt_ro.c libfdt/fdt_wip.c libfdt/fdt_sw.c \
         libfdt/fdt_rw.c libfdt/fdt_strerror.c libfdt/fdt_addresses.c \
         libfdt/fdt_empty_tree.c ) || { echo "FAILED=build-dtc"; exit 1; }
fi
"$DTC" --version

# 2. preprocess + compile DTS
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
gcc -E -P -nostdinc -undef -D__DTS__ -x assembler-with-cpp \
    -I "$SRC/arch/arm64/boot/dts/sprd" \
    -I "$SRC/arch/arm64/boot/dts" \
    -I "$SRC/include" \
    -o "$TMP/pre.dts" "$SRC/arch/arm64/boot/dts/sprd/ums512-1h10-vxtux.dts" \
    || { echo "FAILED=cpp"; exit 1; }

"$DTC" -I dts -O dtb -o "$OUT/ums512-1h10-vxtux.dtb" "$TMP/pre.dts" \
  -Wno-unique_unit_address -Wno-unit_address_vs_reg \
  -Wno-avoid_unnecessary_addr_size -Wno-alias_paths \
  -Wno-graph_child_address -Wno-graph_port -Wno-simple_bus_reg \
  2>&1 | grep -v 'Warning (reg_format)\|Failed prerequisite' || true

[ -s "$OUT/ums512-1h10-vxtux.dtb" ] || { echo "FAILED=dtc"; exit 1; }
echo "[dtb] ditulis $OUT/ums512-1h10-vxtux.dtb ($(stat -c%s "$OUT/ums512-1h10-vxtux.dtb") byte)"
