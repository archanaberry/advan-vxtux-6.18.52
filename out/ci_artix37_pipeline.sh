#!/bin/bash
# ci_artix37_pipeline.sh -- FIX: clock tree sebagai MODUL, dimuat dari
# /vxtux/init SETELAH fbcon hidup (artix37).
#
# JAWABAN "kenapa artix34 masuk fbcon, artix35/36 tidak":
#   * artix34: SPRD_COMMON_CLK/UMS512_CLK =m dan modulnya TIDAK PERNAH
#     dimuat -> semua konsumen (eMMC sdio, i2c, PMIC sc2730, ...) dapat
#     -EPROBE_DEFER selamanya -> tidak ada yang probe -> kernel santai
#     jalan sampai device_initcall, lkfb mendaftar, LAYAR HIDUP, init
#     userspace mencetak pesan.
#   * artix35/36: =y -> provider clock terdaftar sejak initcall -> konsumen
#     langsung probe untuk pertama kalinya -> salah satunya menggantung
#     senyap (bus/perangkat belum siap) SEBELUM fbcon -> layar tetap teks
#     LK, tanpa panic, tanpa tulisan cache.  sprd_clk_probe sendiri polos
#     (hanya registrasi + of_clk_add_hw_provider), jadi pelakunya pasti
#     konsumen clock, bukan driver clock-nya.
#
# PERBAIKAN artix37:
#   * enhanced_config.cfg: SPRD_COMMON_CLK=m, SPRD_UMS512_CLK=m (persis
#     nilai artix34 yang terbukti selalu sampai fbcon).
#   * ci_qemu/initramfs/init.c: load_clock_modules() -- finit_module()
#     /vxtux/ko/clk-sprd.ko lalu /vxtux/ko/ums512-clk.ko SETELAH fbcon
#     hidup.  driver_register() memicu driver_deferred_probe_trigger(),
#     jadi konsumen yang tadinya deferred langsung dicoba ulang; kalau ada
#     yang menggantung, gantungnya SETELAH pesan layar tercetak.
#   * pipeline ini menyuntikkan kedua .ko (artefak v17-built-kos run yang
#     SAMA -> vermagic identik) ke initramfs boot.img di /vxtux/ko/.
#     Artefak itu keluar unsigned, jadi enhanced_config.cfg mematikan
#     CONFIG_MODULE_SIG_FORCE (tanpa itu finit_module menolak EKEYREJECTED).
#   * VXTUX_INITTRACE=n: tracer artix36 dimatikan (find_font dari
#     early_param selalu NULL karena font baru terdaftar di device_initcall,
#     jadi tracer tidak pernah menggambar apa pun).  Jalur render yang
#     dipakai = jalur artix34 yang terbukti benar, satu variabel berubah.
#
# KANAL BACA: layar fbcon langsung (dmesg + "VXTUX-MOD:" + "VXTUX-STEP:")
set -u
cd /e/vxtux_build_repo
REPO=gryzzlaycutzy121/advan-vxtux-6.18.52

echo "[pipeline] cari run terbaru branch test1 ..."
sleep 15
RUN=$(gh run list --repo "$REPO" --branch test1 --limit 1 --json databaseId,status --jq '.[0].databaseId')
echo "RUN=$RUN"
if [ -z "$RUN" ] || [ "$RUN" = "null" ]; then echo "FAILED=no-run"; exit 1; fi

echo "[pipeline] watch run $RUN (max 10 percobaan) ..."
rc=1
for try in 1 2 3 4 5 6 7 8 9 10; do
  gh run watch "$RUN" --repo "$REPO" --exit-status --interval 30
  rc=$?
  echo "WATCH_RC=$rc (percobaan $try)"
  [ "$rc" -eq 0 ] && break
  st=$(gh run view "$RUN" --repo "$REPO" --json status --jq .status 2>/dev/null || echo unknown)
  echo "  status=$st"
  if [ "$st" = "completed" ]; then break; fi
  sleep 30
done
if [ "$rc" -ne 0 ]; then
  echo "FAILED=build"
  gh run view "$RUN" --repo "$REPO" --log-failed 2>&1 | tail -40
  exit 1
fi

rm -rf out/ci_artix37
mkdir -p out/ci_artix37/ko out/artix_ramdisk
gh run download "$RUN" --repo "$REPO" -n v17-Image -D out/ci_artix37 || { echo "FAILED=download-image"; exit 1; }
IMG=$(find out/ci_artix37 -type f -name "Image*" | head -1)
echo "IMG_KETEMU=$IMG"
[ -z "$IMG" ] && { echo "FAILED=no-image"; exit 1; }
cp "$IMG" out/6.18.52-VXTux-UMS512/Image

# --- 0a. konfigurasi efektif build ---------------------------------------
gh run download "$RUN" --repo "$REPO" -n v17-config -D out/ci_artix37/cfg || true
CFG=$(find out/ci_artix37/cfg -type f -name "effective.config" -o -name ".config" 2>/dev/null | head -1)
echo "CFG=$CFG"
if [ -n "$CFG" ]; then
  for want in CONFIG_SPRD_COMMON_CLK=m CONFIG_SPRD_UMS512_CLK=m \
              CONFIG_MODULES=y; do
    if grep -qx "$want" "$CFG"; then
      echo "  config OK : $want"
    else
      echo "FAILED=config $want tidak ada di konfigurasi efektif build"
      grep -E "SPRD_(COMMON|UMS512)_CLK|VXTUX_INITTRACE|MODULE_SIG|^CONFIG_MODULES" "$CFG" || true
      exit 1
    fi
  done
  # tracer HARUS mati: simbol tidak boleh =y ("is not set" atau baris hilang)
  if grep -qx "CONFIG_VXTUX_INITTRACE=y" "$CFG"; then
    echo "FAILED=config CONFIG_VXTUX_INITTRACE masih =y (tracer harus mati)"
    exit 1
  fi
  echo "  config OK : CONFIG_VXTUX_INITTRACE tidak =y (tracer mati)"
  # MODULE_SIG_FORCE harus MATI, kalau tidak modul clock ditolak EKEYREJECTED
  if grep -qx "CONFIG_MODULE_SIG_FORCE=y" "$CFG"; then
    echo "FAILED=config CONFIG_MODULE_SIG_FORCE masih =y (modul unsigned akan ditolak)"
    exit 1
  fi
  echo "  config OK : CONFIG_MODULE_SIG_FORCE tidak =y (modul unsigned boleh dimuat)"
else
  echo "  (artefak v17-config tidak ada -- verifikasi lewat .ko + Image)"
fi

# --- 0b. cek instrumen di Image -------------------------------------------
echo "[cek] string di Image:"
for s in "vxtux-fbmem" "vxtux-lkfb" "vxtuxfb=" "vxtux-fr"; do
  n=$(grep -c -a -F "$s" out/6.18.52-VXTux-UMS512/Image || true)
  echo "  ada      : $s -> $n"
done
for s in "VXTUX-PROBE" "vxtux-beacon" "VXTUX-ITRACE"; do
  n=$(grep -c -a -F "$s" out/6.18.52-VXTux-UMS512/Image || true)
  echo "  harus 0  : $s -> $n"
done
n=$(grep -c -a -F "VXTUX-ITRACE" out/6.18.52-VXTux-UMS512/Image || true)
[ "$n" -ne 0 ] && { echo "FAILED=tracer initcall masih ikut ke Image (harus mati)"; exit 1; }
# clock =m -> provider TIDAK boleh builtin lagi di Image
n=$(grep -c -a -F "sprd,ums512-apapb-gate" out/6.18.52-VXTux-UMS512/Image || true)
echo "  info     : sprd,ums512-apapb-gate di Image -> $n (harus 0, sekarang modul)"
if [ "$n" -ne 0 ]; then echo "FAILED=clock masih builtin di Image padahal harus =m"; exit 1; fi
bad=$(grep -c -a -E "gagal|terdaftar|direservasi|tidak |dengan |untuk " out/6.18.52-VXTux-UMS512/Image || true)
echo "  kata Indonesia di Image -> $bad (harus 0)"

# --- 0c. .ko dari run yang sama (tanda tangan cocok) ----------------------
gh run download "$RUN" --repo "$REPO" -n v17-built-kos -D out/ci_artix37/ko || { echo "FAILED=download-ko"; exit 1; }
KOTAR=$(find out/ci_artix37/ko -name '*.tar.gz' | head -1)
echo "KOTAR=$KOTAR"
[ -z "$KOTAR" ] && { echo "FAILED=no-ko-tarball"; exit 1; }
mkdir -p out/ci_artix37/kox
tar -xzf "$KOTAR" -C out/ci_artix37/kox
CLKKO=$(find out/ci_artix37/kox -name 'clk-sprd.ko' | head -1)
UMSKO=$(find out/ci_artix37/kox -name 'ums512-clk.ko' | head -1)
echo "CLKKO=$CLKKO"
echo "UMSKO=$UMSKO"
[ -z "$CLKKO" ] || [ -z "$UMSKO" ] && { echo "FAILED=ko clock tidak ditemukan di tarball"; exit 1; }
for k in "$CLKKO" "$UMSKO"; do
  printf '  %s: %s byte,' "$(basename "$k")" "$(stat -c%s "$k")"
  if grep -q -a -F '~Module signature appended~' "$k"; then
    printf ' signed'
  else
    printf ' unsigned (diterima: MODULE_SIG_FORCE mati)'
  fi
  if grep -q -a -F '6.18.52-VXTux-UMS512' "$k"; then echo ', vermagic OK'; else echo "FAILED=vermagic $k tidak cocok"; exit 1; fi
done
cp "$CLKKO" out/artix_ramdisk/clk-sprd.ko
cp "$UMSKO" out/artix_ramdisk/ums512-clk.ko

# --- 1. initramfs: validasi init + suntik /vxtux/ko/*.ko, lalu gzip ulang -
/c/Python*/python.exe - <<'PYEOF'
import zlib, gzip, sys, struct

img = open('out/6.18.52-VXTux-UMS512/Image', 'rb').read()
assert img[0x38:0x3C] == b'ARM\x64', 'magic ARM64 salah'
print('magic ARM64 OK, ukuran:', len(img))

blob, i = None, 0
while True:
    i = img.find(b'\x1f\x8b\x08', i)
    if i < 0:
        break
    try:
        do = zlib.decompressobj(16 + zlib.MAX_WBITS)
        out = do.decompress(img[i:i + 8 * 1024 * 1024])
        do.flush()
        if out[:6] in (b'070701', b'070702') and b'TRAILER!!!' in out:
            consumed = len(img[i:i + 8 * 1024 * 1024]) - len(do.unused_data)
            blob = img[i:i + consumed]
            break
    except Exception:
        pass
    i += 1
if not blob:
    sys.exit('FAILED: initramfs embedded tidak ditemukan di Image')

p, names, elf_ok, inggris_ok, test_ok, wait_ok, mod_ok = 0, [], False, False, False, False, False
trailer_at = -1
while p + 110 < len(out) and out[p:p + 6] in (b'070701', b'070702'):
    fsize = int(out[p + 54:p + 62], 16)
    namesize = int(out[p + 94:p + 102], 16)
    name = out[p + 110:p + 110 + namesize - 1].decode('latin1')
    data = p + 110 + namesize
    aligned = (data + 3) & ~3
    if name == 'vxtux/init':
        d = out[aligned:aligned + fsize]
        elf_ok = (d[:4] == b'\x7fELF' and
                  struct.unpack('<H', d[18:20])[0] == 183 and
                  b'chroot OK' in d)
        inggris_ok = (b'1/5 partition list' in d and
                      b'daftar partisi' not in d and
                      b'gagal' not in d and b'akhir' not in d)
        test_ok = (b'VXTUX-WRITE-TEST' in d and b'readback verified' in d)
        wait_ok = (b'waiting for eMMC' in d and
                   b'storage ready after' in d and b'mknod' in d)
        mod_ok = (b'VXTUX-MOD:' in d and b'/vxtux/ko/clk-sprd.ko' in d and
                  b'/vxtux/ko/ums512-clk.ko' in d)
        print('vxtux/init: ELF=%s aarch64=%s dinit=%s inggris=%s writetest=%s wait=%s insmod=%s ukuran=%d'
              % (d[:4] == b'\x7fELF', struct.unpack('<H', d[18:20])[0] == 183,
                 b'chroot OK' in d, inggris_ok, test_ok, wait_ok, mod_ok, fsize))
    if name == 'TRAILER!!!':
        trailer_at = p
        names.append(name)
        break
    names.append(name)
    p = aligned + fsize
    p = (p + 3) & ~3
print('entri:', names)
if 'vxtux/init' not in names:
    sys.exit('FAILED: /vxtux/init tidak ada di initramfs embedded')
if not elf_ok:
    sys.exit('FAILED: initramfs embedded bukan ELF aarch64 dinit yang benar')
if not inggris_ok:
    sys.exit('FAILED: initramfs embedded masih memakai teks Indonesia')
if not test_ok:
    sys.exit('FAILED: initramfs embedded tidak memuat VXTUX-WRITE-TEST')
if not wait_ok:
    sys.exit('FAILED: initramfs embedded tidak memuat wait_for_storage')
if not mod_ok:
    sys.exit('FAILED: initramfs embedded tidak memuat load_clock_modules (VXTUX-MOD)')
if trailer_at < 0:
    sys.exit('FAILED: TRAILER cpio tidak ditemukan')

# --- suntikkan .ko ke dalam cpio (newc) di /vxtux/ko/ --------------------
def newc_entry(name, data, ino, mode=0o100644):
    namez = name.encode() + b'\0'
    fields = [ino, mode, 0, 0, 1, 0, len(data), 0, 0, 0, 0, len(namez), 0]
    hdr = b'070701' + b''.join(b'%08X' % f for f in fields)
    ent = hdr + namez
    ent += b'\0' * ((4 - len(ent) % 4) % 4)
    ent += data
    ent += b'\0' * ((4 - len(ent) % 4) % 4)
    return ent

ko_paths = [('vxtux/ko/clk-sprd.ko', 'out/artix_ramdisk/clk-sprd.ko'),
            ('vxtux/ko/ums512-clk.ko', 'out/artix_ramdisk/ums512-clk.ko')]
entries = b''
ino = 900001
for arcname, path in ko_paths:
    data = open(path, 'rb').read()
    entries += newc_entry(arcname, data, ino)
    ino += 1
    print('suntik cpio: %-28s %d byte' % (arcname, len(data)))
newcpio = out[:trailer_at] + entries + out[trailer_at:]

gz = gzip.compress(newcpio, 9, mtime=0)
open('out/artix_ramdisk/ramdisk', 'wb').write(gz)
print('ramdisk ditulis ulang: out/artix_ramdisk/ramdisk = %d byte (gzip cpio, +2 ko)'
      % len(gz))
PYEOF
if [ $? -ne 0 ]; then echo "FAILED=ramdisk"; exit 1; fi

# --- 1b. verifikasi ramdisk final memuat ko ------------------------------
/c/Python*/python.exe - <<'PYEOF'
import gzip, sys
d = gzip.decompress(open('out/artix_ramdisk/ramdisk', 'rb').read())
for need in (b'vxtux/ko/clk-sprd.ko\0', b'vxtux/ko/ums512-clk.ko\0',
             b'vxtux/init\0'):
    ok = need in d
    print('  %-28s %s' % (need[:-1].decode(), 'ADA' if ok else 'HILANG'))
    if not ok:
        sys.exit('FAILED=ramdisk final kehilangan entri')
print('ramdisk verifikasi OK, isi cpio:', len(d), 'byte')
PYEOF
if [ $? -ne 0 ]; then echo "FAILED=ramdisk-verify"; exit 1; fi

# --- 2. pack boot_artix37.img ---------------------------------------------
/c/Python*/python.exe ci_qemu/mk_boot_artix.py \
  out/6.18.52-VXTux-UMS512/Image \
  out/6.18.52-VXTux-UMS512/boot_artix37.img \
  --base out/base_boot --ramdisk out/artix_ramdisk || { echo "FAILED=pack"; exit 1; }

ls -l out/6.18.52-VXTux-UMS512/boot_artix37.img
md5sum out/6.18.52-VXTux-UMS512/boot_artix37.img
echo "DONE=1"
