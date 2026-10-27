#!/system/bin/sh
# ============================================================
#  start_hypr_test.sh -- jalankan Hyprland dari chroot Artix
#  Android (surfaceflinger + zygote/ART) sudah dimatikan,
#  jadi DRM card0 bebas untuk dijadikan master oleh Hyprland.
#  Konsol kernel tetap di ttyS1, jadi tty1 bebas untuk VT Linux.
#  Dijalankan sebagai root:  adb shell su -c <script>
# ============================================================
R=/data/data/com.termux/files/usr/var/lib/proot-distro/containers/artix/rootfs

mkdir -p "$R/run/user/0" "$R/tmp"
chmod 700 "$R/run/user/0"

# Hyprland butuh VT + XDG_RUNTIME_DIR; stdin/stdout diarahkan ke tty1
chroot "$R" /bin/sh -c 'export XDG_RUNTIME_DIR=/run/user/0 HOME=/root TERM=linux LANG=C.UTF-8; setsid /usr/bin/Hyprland </dev/tty1 >/tmp/hyprland.log 2>&1 &'

sleep 10

echo "=== proses hyprland ==="
ps -A 2>/dev/null | grep -iE 'hypr|Xwayland' | head -10
echo "=== isi log ==="
head -50 "$R/tmp/hyprland.log" 2>/dev/null
echo "=== socket wayland ==="
ls -l "$R/run/user/0/" 2>/dev/null | head -15
