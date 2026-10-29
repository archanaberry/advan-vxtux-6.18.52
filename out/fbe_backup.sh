#!/system/bin/sh
# fbe_backup.sh -- jalankan DI TABLET (Android asli, root/Magisk), bukan di PC.
#
#   adb push out/fbe_backup.sh /data/local/tmp/
#   adb shell su -c "sh /data/local/tmp/fbe_backup.sh"
#
# Yang dikerjakan, berurutan:
#   1. merekam keadaan kripto (getprop, vold, dm, mount) -> bukti, bukan tebakan
#   2. menyalin berkas kunci /metadata apa adanya (blob) + md5-nya
#   3. mencatat kebijakan fscrypt tiap direktori penting (xattr)
#   4. opsional: menyalin /data ke penyimpanan luar (BUTUH ruang + waktu)
#
# Kenapa tidak "mendekripsi": kunci mentah tidak ada di eMMC.  Yang ada di
# /metadata adalah kunci terbungkus AES-GCM (92 byte) oleh kunci milik TEE
# Keymaster, dan blob Keymaster (218 byte) yang hanya bisa dibuka Trusty.
# Jadi yang berguna diselamatkan adalah DATA-nya (langkah 4), bukan kuncinya.
#
# Hasil: /sdcard/fbe_backup/<tanggal>/  (aman dibaca dari PC lewat MTP)
set -u

if [ "$(id -u)" != "0" ]; then
    echo "[X] butuh root. Jalankan: su -c \"sh $0\""
    exit 1
fi

STAMP=$(date +%Y%m%d-%H%M%S)
OUT=/sdcard/fbe_backup/$STAMP
mkdir -p "$OUT" || { echo "[X] tidak bisa menulis $OUT"; exit 1; }

say() { echo "$*" | tee -a "$OUT/report.txt"; }

say "=== FBE BACKUP $STAMP ==="
say ""
say "--- 1. keadaan kripto ---"
for p in ro.crypto.state ro.crypto.type ro.crypto.fs_crypto_blkdev \
         ro.crypto.volume.filenames_mode vold.decrypt \
         ro.hardware ro.build.version.release ro.build.fingerprint; do
    say "  $(getprop $p | sed "s/^/$p = /")"
done
say "  kernel      : $(uname -r)"
say "  selinux     : $(getenforce 2>/dev/null)"
say "  ce_available: $(getprop sys.user.0.ce_available)"
say ""
say "--- 2. mount /data dan /metadata ---"
mount | grep -E ' /data | /metadata | /mnt/vendor ' | sed 's/^/  /' | tee -a "$OUT/report.txt"
say ""
say "--- 3. berkas kunci metadata (blob, bukan kunci mentah) ---"
K=/metadata/vold/metadata_encryption/key
if [ -d "$K" ]; then
    ls -l "$K" | sed 's/^/  /' | tee -a "$OUT/report.txt"
    md5sum "$K"/* 2>/dev/null | sed 's/^/  /' >> "$OUT/report.txt"
    mkdir -p "$OUT/metadata_key"
    cp "$K"/* "$OUT/metadata_key/" 2>/dev/null
    for f in version stretching; do
        [ -f "$K/$f" ] && say "  $f = $(cat "$K/$f")"
    done
    say "  (salinan blob -> $OUT/metadata_key/)"
else
    say "  [i] $K tidak ada (metadata belum FBE / beda versi Android)"
fi
say ""
say "--- 4. kunci per-user (di dalam /data, terenkripsi metadata key) ---"
if [ -d /data/misc/vold/user_keys ]; then
    find /data/misc/vold/user_keys -maxdepth 3 -type f -exec ls -l {} \; \
        2>/dev/null | sed 's/^/  /' | tee -a "$OUT/user_keys.txt"
else
    say "  [i] /data/misc/vold/user_keys tidak terlihat"
fi
say ""
say "--- 5. kebijakan fscrypt pada direktori penting ---"
for d in /data /data/misc /data/system /data/user /data/media; do
    [ -d "$d" ] || continue
    pol=$(ls -ld "$d" 2>/dev/null)
    say "  $pol"
    getfattr -d -m - "$d" 2>/dev/null | grep -i encryption | sed 's/^/    /' \
        | tee -a "$OUT/report.txt"
done
say ""
say "--- 6. tabel device-mapper (dm-default-key / dm-crypt) ---"
dmctl list 2>/dev/null | sed 's/^/  /' || \
    ls /dev/block/dm-* 2>/dev/null | sed 's/^/  /'
say ""
say "=== selesai: $OUT (report.txt + metadata_key/ + user_keys.txt) ==="
say ""
say "LANGKAH DATA (paling berguna, tanpa kripto):"
say "  tar -C /data -cf /sdcard/fbe_backup/data_$STAMP.tar \\"
say "      --exclude=data/media/0/Android/obb media/0 DCIM Pictures Documents \\"
say "      Download Movies Music 2>/dev/null"
say "  (jalankan manual; sengaja tidak otomatis supaya tidak mengisi storage"
say "   tanpa kamu setujui)"
