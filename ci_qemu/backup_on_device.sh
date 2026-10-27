#!/system/bin/sh
# ============================================================
#  backup_on_device.sh -- arsip /data -> stdout (tar)
#  Dijalankan sebagai root:  adb exec-out su -c <script>
#
#  Catatan:
#   * TANPA --selinux: getfilecon gagal (EINVAL) di f2fs ini,
#     jadi konteks label tidak ikut tersimpan. Saat restore,
#     jalankan:  restorecon -RF /data
#   * /data/user TIDAK diikutkan: itu bind-mount duplikat
#     dari /data/data/0 (isinya sama persis).
#   * /data/user_de (DE storage, 328M) tetap ikut.
# ============================================================
exec tar -cpf - -C /data \
  adb anr apex app app-asec app-ephemeral app-lib app-private app-staging \
  backup bootanim bootchart cache dalvik-cache data drm fonts fulldump \
  gsi gsi_persistent_data incremental local log lost+found media mediadrm \
  minidump misc misc_ce misc_de nfc oppo ota ota_package per_boot preloads \
  property resource-cache rollback rollback-history rollback-observer \
  server_configurable_flags ss system system_ce system_de tombstones \
  unencrypted user_de vendor vendor_ce vendor_de ylog
