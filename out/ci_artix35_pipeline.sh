#!/bin/bash
# ci_artix35_pipeline.sh -- PERBAIKAN AKAR MASALAH: driver clock UMS512 =y.
#
# BEDAH artix34 (penyebab semua kegagalan eMMC sejak build 20 Okt):
#   * stock.dtb: host eMMC sdio@71400000 (sprd,sdhci-r11) butuh clock
#     "sdio" dari /soc/clock-controller@20200000 (sprd,ums512-ap-clk) dan
#     clock "enable" dari /soc/apapb-gate (sprd,ums512-apapb-gate) --
#     KEDUANYA didaftarkan oleh drivers/clk/sprd/ums512-clk.c.
#   * .config lama: CONFIG_SPRD_UMS512_CLK=m (sejak commit 80146f0f6 /
#     42b8db22f, 20 Okt) -- modul tidak pernah dimuat di boot initramfs
#     tanpa userspace, jadi kedua clock tidak pernah ada.
#   * sdhci_sprd_probe(): devm_clk_get("sdio") / ("enable") gagal ->
#     return -EPROBE_DEFER -> host eMMC tidak PERNAH probe -> disk tidak
#     terdaftar -> /dev/mmcblk0p49 ENOENT (terlihat di layar artix34) ->
#     pstore_blk, flightrec dan /vxtux/init semuanya gagal serentak ->
#     28 dump cache nol byte + rootfs tak pernah ke-mount.
#
# PERBAIKAN:
#   1. .config + enhanced_config.cfg: SPRD_COMMON_CLK=y, SPRD_UMS512_CLK=y
#      (clock tree Unisoc masuk ke vmlinux).
#   2. initramfs /vxtux/init: wait_for_storage() -- tunggu mmcblk* muncul
#      (maks 180 s), layar diperbarui tiap 10 s dengan jumlah partisi +
#      petunjuk dmesg (sdio/mmc/-517/defer), lalu fallback mknod dari
#      major/minor sebelum uji tulis/laporan/pindai ext4.
#   3. vxtux_flightrec.c: kegagalan yang sama dicetak ulang tiap 10 s
#      (keadaan terkini selalu terlihat di fbcon); bila disk mmcblk0 tidak
#      terdaftar, nama disk yang ada ikut dicetak.
#
# KANAL PEMBACAAN (dump_artix35_logonly): pstore_blk @0 | laporan VXTUXLOG1
# @4 MiB | zona uji VXTUXWRT @6 MiB | flightrec VXTUXFR1 @8 MiB.
set -u
cd /e/vxtux_build_repo
REPO=gryzzlaycutzy121/advan-vxtux-6.18.52

echo "[pipeline] cari run terbaru branch test1 ..."
sleep 15
RUN=$(gh run list --repo "$REPO" --branch test1 --limit 1 --json databaseId,status --jq '.[0].databaseId')
echo "RUN=$RUN"
if [ -z "$RUN" ] || [ "$RUN" = "null" ]; then echo "FAILED=no-run"; exit 1; fi

echo "[pipeline] watch run $RUN (max 10 percobaan, jaringan suka putus) ..."
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

rm -rf out/ci_artix35
mkdir -p out/ci_artix35
gh run download "$RUN" --repo "$REPO" -n v17-Image -D out/ci_artix35 || { echo "FAILED=download"; exit 1; }
IMG=$(find out/ci_artix35 -type f -name "Image*" | head -1)
echo "IMG_KETEMU=$IMG"
if [ -z "$IMG" ]; then echo "FAILED=no-image"; exit 1; fi
cp "$IMG" out/6.18.52-VXTux-UMS512/Image

# --- 0a. konfigurasi efektif build: clock tree Harus builtin ---------------
gh run download "$RUN" --repo "$REPO" -n v17-config -D out/ci_artix35/cfg || true
CFG=$(find out/ci_artix35/cfg -type f -name "effective.config" -o -name ".config" 2>/dev/null | head -1)
echo "CFG=$CFG"
if [ -n "$CFG" ]; then
  for want in CONFIG_SPRD_COMMON_CLK=y CONFIG_SPRD_UMS512_CLK=y; do
    if grep -qx "$want" "$CFG"; then
      echo "  config OK : $want"
    else
      echo "FAILED=config $want tidak ada di konfigurasi efektif build"
      grep -E "SPRD_(COMMON|UMS512)_CLK" "$CFG" || true
      exit 1
    fi
  done
else
  echo "  (artefak v17-config tidak ditemukan -- lanjut, verifikasi lewat string Image)"
fi

# --- 0b. cek instrumen di Image --------------------------------------------
echo "[cek] string di Image:"
for s in "vxtux-fbmem" "vxtux-lkfb" "vxtuxfb=" "vxtux-fr" "readback after write" \
         "write channel recovered" \
         "sprd,ums512-apapb-gate" "sprd,ums512-ap-clk" \
         "waiting for eMMC" "storage ready after" \
         "disk mmcblk0 not registered" "registered disk["; do
  n=$(grep -c -a -F "$s" out/6.18.52-VXTux-UMS512/Image || true)
  echo  "  ada      : $s -> $n"
done
for s in "VXTUX-PROBE" "vxtux-beacon"; do
  n=$(grep -c -a -F "$s" out/6.18.52-VXTux-UMS512/Image || true)
  echo  "  harus 0  : $s -> $n"
done
# gate keras: clock driver HARUS builtin (kalau =m lagi, string di atas 0)
if ! grep -q -a -F "sprd,ums512-apapb-gate" out/6.18.52-VXTux-UMS512/Image; then
  echo "FAILED=clock UMS512 tidak builtin di Image (sprd,ums512-apapb-gate tidak ada)"
  exit 1
fi
if ! grep -q -a -F "waiting for eMMC" out/6.18.52-VXTux-UMS512/Image; then
  echo "FAILED=initramfs tanpa wait_for_storage artix35"
  exit 1
fi
# teks layar harus Inggris: tidak boleh ada kata Indonesia di string yang dicetak
bad=$(grep -c -a -E "gagal|terdaftar|direservasi|tidak |dengan |untuk " out/6.18.52-VXTux-UMS512/Image || true)
echo "  kata Indonesia di Image -> $bad (harus 0)"

# --- 1. ramdisk dibuat ulang dari initramfs embedded di dalam Image --------
mkdir -p out/artix_ramdisk
/c/Python*/python.exe - <<'PYEOF'
import zlib, os, sys, struct

img = open('out/6.18.52-VXTux-UMS512/Image', 'rb').read()
assert img[0x38:0x3C] == b'ARM\x64', 'magic ARM64 salah'
print('magic ARM64 OK, ukuran:', len(img))

blob, off = None, -1
i = 0
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
            blob, off = img[i:i + consumed], i
            break
    except Exception:
        pass
    i += 1
if not blob:
    sys.exit('FAILED: initramfs embedded tidak ditemukan di Image')

# verifikasi STRUKTUR STANDAR (pad nama 4 byte) + isi, atas `out` (cpio
# hasil dekompresi) -- blob yang ditulis tetap gzip-nya.
p, names, elf_ok, inggris_ok, test_ok, wait_ok = 0, [], False, False, False, False
while p + 110 < len(out) and out[p:p + 6] in (b'070701', b'070702'):
    fsize = int(out[p + 54:p + 62], 16)
    namesize = int(out[p + 94:p + 102], 16)
    name = out[p + 110:p + 110 + namesize - 1].decode('latin1')
    data = p + 110 + namesize
    aligned = (data + 3) & ~3
    if name == 'vxtux/init':
        d = out[aligned:aligned + fsize]
        elf_ok = (d[:4] == b'\x7fELF' and
                  struct.unpack('<H', d[18:20])[0] == 183 and   # EM_AARCH64
                  b'chroot OK' in d)
        # teks layar init harus Inggris DAN memuat uji tulis artix34
        inggris_ok = (b'1/5 partition list' in d and
                      b'daftar partisi' not in d and
                      b'gagal' not in d and b'akhir' not in d)
        test_ok = (b'VXTUX-WRITE-TEST' in d and
                   b'readback verified' in d)
        # loop tunggu eMMC + fallback mknod (artix35)
        wait_ok = (b'waiting for eMMC' in d and
                   b'storage ready after' in d and
                   b'mknod' in d)
        print('vxtux/init: ELF=%s aarch64=%s dinit=%s inggris=%s writetest=%s wait=%s ukuran=%d'
              % (d[:4] == b'\x7fELF', struct.unpack('<H', d[18:20])[0] == 183,
                 b'chroot OK' in d, inggris_ok, test_ok, wait_ok, fsize))
    names.append(name)
    p = aligned + fsize
    p = (p + 3) & ~3
    if name == 'TRAILER!!!':
        break
print('entri:', names)
if 'vxtux/init' not in names:
    sys.exit('FAILED: /vxtux/init tidak ada di initramfs embedded')
if not elf_ok:
    sys.exit('FAILED: initramfs embedded bukan ELF aarch64 dinit yang benar')
if not inggris_ok:
    sys.exit('FAILED: initramfs embedded masih memakai teks Indonesia')
if not test_ok:
    sys.exit('FAILED: initramfs embedded tidak memuat VXTUX-WRITE-TEST artix34')
if not wait_ok:
    sys.exit('FAILED: initramfs embedded tidak memuat wait_for_storage artix35')

open('out/artix_ramdisk/ramdisk', 'wb').write(blob)
print('ramdisk ditulis ulang: out/artix_ramdisk/ramdisk = %d byte (gzip cpio)'
      % len(blob))
PYEOF
if [ $? -ne 0 ]; then echo "FAILED=ramdisk"; exit 1; fi

# --- 2. pack boot_artix35.img ----------------------------------------------
/c/Python*/python.exe ci_qemu/mk_boot_artix.py \
  out/6.18.52-VXTux-UMS512/Image \
  out/6.18.52-VXTux-UMS512/boot_artix35.img \
  --base out/base_boot --ramdisk out/artix_ramdisk || { echo "FAILED=pack"; exit 1; }

ls -l out/6.18.52-VXTux-UMS512/boot_artix35.img
md5sum out/6.18.52-VXTux-UMS512/boot_artix35.img
echo "DONE=1"
