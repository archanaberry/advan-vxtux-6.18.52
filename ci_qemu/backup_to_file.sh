#!/system/bin/sh
# ============================================================
#  backup_to_file.sh -- tulis arsip /data ke BERKAS di perangkat
#  (tar lokal f2fs, cepat), nanti diambil dengan `adb pull`.
#
#  Output di /data/user/ karena:
#    * /data/user TIDAK ikut tar (bind-mount duplikat /data/data/0)
#    * partisi sama, ruang cukup (66G kosong)
#
#  Selesai ditandai berkas /data/user/backup.done.
#  Tanpa --selinux (getfilecon gagal di f2fs); saat restore
#  jalankan:  restorecon -RF /data
# ============================================================
OUT=/data/user/dataadvan.tar
DONE=/data/user/backup.done
rm -f "$DONE" "$OUT"

tar -cpf "$OUT" -C /data \
  adb anr apex app app-asec app-ephemeral app-lib app-private app-staging \
  backup bootanim bootchart cache dalvik-cache data drm fonts fulldump \
  gsi gsi_persistent_data incremental local log lost+found media mediadrm \
  minidump misc misc_ce misc_de nfc oppo ota ota_package per_boot preloads \
  property resource-cache rollback rollback-history rollback-observer \
  server_configurable_flags ss system system_ce system_de tombstones \
  unencrypted user_de vendor vendor_ce vendor_de ylog
RC=$?
echo "rc=$RC size=$(stat -c %s "$OUT" 2>/dev/null) etime=$(cut -d. -f1 /proc/uptime)" > "$DONE"
