#!/bin/bash
# ci_artix34_pipeline.sh -- KANAL TULIS TERINSTRUMENTASI: kenapa cache nol byte.
#
# APA YANG DIUBAH DI KERNEL (commit sesudah artix33):
#   1. vxtux_flightrec.c: setiap kegagalan yang berbeda (open by devt,
#      open /dev path, write, readback) dicetak SEKALI ke dmesg dengan
#      errno-nya -> fbcon memutarnya kembali, jadi SEBAB kegagalan terlihat
#      di layar tanpa dump.  Setiap tulisan sukses diverifikasi baca-balik
#      8 byte (magic VXTUXFR1); ada pesan "write channel recovered" kalau
#      kanal sempat pulih.  Fallback kedua: bila gendisk/devt ditolak,
#      coba lagi lewat node /dev/mmcblk0p49 (devtmpfs).
#   2. initramfs /vxtux/init (ci_qemu/initramfs/init.c):
#      - VXTUX-WRITE-TEST: uji tulis-baca-balik nyata ke zona 6 MiB partisi
#        cache (celah kosong antara pstore ~0, laporan 4 MiB, flightrec
#        8 MiB), hasilnya dicetak ke console (fbcon) + laporan:
#          VXTUX-WRITE-TEST: OK open+write+readback on /dev/mmcblk0p49
#        atau FAIL open/write/readback dengan errno.
#      - write_report_to() kini membuka O_RDWR dan MENGAMBACA BALIK kepala
#        laporan "### VXTUXLOG1" sebelum menyatakan OK.
#      - daftar /proc/partitions ikut dicetak ke LAYAR (kelihatan langsung
#        apakah p49 ada).
#      - sisa teks Indonesia ("akhir dmesg", "akhir /proc/partitions")
#        diganti Inggris.
#
# KANAL PEMBACAAN TETAP SAMA (dump artix34_logonly):
#   pstore_blk @ 0 MiB | laporan init VXTUXLOG1 @ 4 MiB | zona uji @ 6 MiB |
#   flightrec VXTUXFR1 @ 8 MiB -- semuanya di partisi cache.
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

rm -rf out/ci_artix34
mkdir -p out/ci_artix34
gh run download "$RUN" --repo "$REPO" -n v17-Image -D out/ci_artix34 || { echo "FAILED=download"; exit 1; }
IMG=$(find out/ci_artix34 -type f -name "Image*" | head -1)
echo "IMG_KETEMU=$IMG"
if [ -z "$IMG" ]; then echo "FAILED=no-image"; exit 1; fi
cp "$IMG" out/6.18.52-VXTux-UMS512/Image

# --- 0. cek instrumen: yang lama HARUS hilang, yang baru HARUS ada ----------
echo "[cek] string di Image:"
for s in "vxtux-fbmem" "vxtux-lkfb" "vxtuxfb=" "vxtux-fr" "readback after write" \
         "write channel recovered"; do
  n=$(grep -c -a -F "$s" out/6.18.52-VXTux-UMS512/Image || true)
  echo  "  ada      : $s -> $n"
done
for s in "VXTUX-PROBE" "vxtux-beacon"; do
  n=$(grep -c -a -F "$s" out/6.18.52-VXTux-UMS512/Image || true)
  echo  "  harus 0  : $s -> $n"
done
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
p, names, elf_ok, inggris_ok, test_ok = 0, [], False, False, False
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
        print('vxtux/init: ELF=%s aarch64=%s dinit=%s inggris=%s writetest=%s ukuran=%d'
              % (d[:4] == b'\x7fELF', struct.unpack('<H', d[18:20])[0] == 183,
                 b'chroot OK' in d, inggris_ok, test_ok, fsize))
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

open('out/artix_ramdisk/ramdisk', 'wb').write(blob)
print('ramdisk ditulis ulang: out/artix_ramdisk/ramdisk = %d byte (gzip cpio)'
      % len(blob))
PYEOF
if [ $? -ne 0 ]; then echo "FAILED=ramdisk"; exit 1; fi

# --- 2. pack boot_artix34.img ----------------------------------------------
/c/Python*/python.exe ci_qemu/mk_boot_artix.py \
  out/6.18.52-VXTux-UMS512/Image \
  out/6.18.52-VXTux-UMS512/boot_artix34.img \
  --base out/base_boot --ramdisk out/artix_ramdisk || { echo "FAILED=pack"; exit 1; }

ls -l out/6.18.52-VXTux-UMS512/boot_artix34.img
md5sum out/6.18.52-VXTux-UMS512/boot_artix34.img
echo "DONE=1"
