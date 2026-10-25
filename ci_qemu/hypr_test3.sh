#!/system/bin/sh
# ============================================================
#  hypr_test3.sh -- tes lengkap dalam satu jendela waktu
#  1. hentikan Android (SF + zygote/ART) -> DRM card0 bebas
#  2. jalankan Hyprland dengan raster software (llvmpipe/kms_swrast)
#  3. kalau Hyprland tetap gagal -> sway dengan WLR_RENDERER=pixman
#     (tanpa EGL sama sekali) untuk membuktikan jalur KMS ke panel
#  4. screenshot grim sebagai bukti
#  Catatan: setelah Android mati, perangkat reboot sendiri dalam
#  ~6 menit, jadi semua langkah harus cepat.
# ============================================================
LOG=/data/local/tmp/hypr_test3.log
R=/data/data/com.termux/files/usr/var/lib/proot-distro/containers/artix/rootfs
: > "$LOG"
say() { echo "[$(cut -d. -f1 /proc/uptime)s] $*" >> "$LOG"; }

say "MULAI SF=$(pidof surfaceflinger) ZYG=$(pidof zygote64)"

mountpoint -q "$R/proc" 2>/dev/null || mount -t proc proc "$R/proc"
mountpoint -q "$R/sys" 2>/dev/null || mount -t sysfs sysfs "$R/sys"
mountpoint -q "$R/dev" 2>/dev/null || mount --bind /dev "$R/dev"
mountpoint -q "$R/dev/pts" 2>/dev/null || mount -t devpts devpts "$R/dev/pts"
[ -e "$R/dev/dri/card0" ] && say "card0 TERLIHAT di chroot" || say "PERINGATAN: card0 tak terlihat"

mkdir -p "$R/run/user/0" "$R/tmp"
chmod 700 "$R/run/user/0"

# --- 1. hentikan Android ---
setprop ctl.stop surfaceflinger
setprop ctl.stop zygote
setprop ctl.stop zygote_secondary
sleep 2
say "Android DIHENTIKAN: SF=$(pidof surfaceflinger) ZYG=$(pidof zygote64)"

# --- 2. peluncur Hyprland (env diverifikasi di log) ---
cat > "$R/tmp/launch3.sh" << 'EOF'
#!/bin/sh
export PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
export XDG_RUNTIME_DIR=/run/user/0 HOME=/root TERM=linux LANG=C.UTF-8
export LIBGL_ALWAYS_SOFTWARE=1 GALLIUM_DRIVER=llvmpipe MESA_LOADER_DRIVER_OVERRIDE=kms_swrast
mkdir -p /run/user/0 && chmod 700 /run/user/0
echo "=== env ==="
env | grep -E 'MESA|GALLIUM|LIBGL|EGL|XDG_RUNTIME'
echo "=== launch Hyprland ==="
exec /usr/bin/Hyprland --i-am-really-stupid > /tmp/hyprland.log 2>&1
EOF
chmod 755 "$R/tmp/launch3.sh"
setsid chroot "$R" /tmp/launch3.sh < /dev/null > "$R/tmp/launch3.out" 2>&1 &
sleep 14

NPROCS=$(ps -A 2>/dev/null | grep -ci hypr)
say "hyprland procs=$NPROCS"
say "--- launch3.out (env) ---"
head -30 "$R/tmp/launch3.out" >> "$LOG" 2>&1
say "--- hyprland.log tail ---"
tail -25 "$R/tmp/hyprland.log" >> "$LOG" 2>&1

# --- 3. kalau Hyprland gagal -> sway pixman (tanpa EGL) ---
if [ "$NPROCS" -eq 0 ]; then
    say "Hyprland gagal -> coba SWAY + WLR_RENDERER=pixman"
    cat > "$R/tmp/sway.sh" << 'EOS'
#!/bin/sh
export PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
export XDG_RUNTIME_DIR=/run/user/0 HOME=/root TERM=linux
export WLR_RENDERER=pixman WLR_BACKENDS=drm WLR_NO_HARDWARE_CURSORS=1
mkdir -p /run/user/0 && chmod 700 /run/user/0
[ -f /root/.config/sway/config ] || { mkdir -p /root/.config/sway; echo 'output * bg #003355 solid_color' > /root/.config/sway/config; }
echo "=== launch sway (pixman) ==="
exec /usr/bin/sway > /tmp/sway.log 2>&1
EOS
    chmod 755 "$R/tmp/sway.sh"
    setsid chroot "$R" /tmp/sway.sh < /dev/null > "$R/tmp/sway.out" 2>&1 &
    sleep 12
    say "sway procs=$(ps -A 2>/dev/null | grep -cE '[s]way')"
    say "--- sway.log tail ---"
    tail -25 "$R/tmp/sway.log" >> "$LOG" 2>&1
fi

# --- 4. screenshot bukti ---
say "GRIM..."
chroot "$R" /bin/sh -c 'export PATH=/usr/bin:/bin XDG_RUNTIME_DIR=/run/user/0; W=$(ls /run/user/0 | grep "^wayland-" | head -1); echo "socket=$W"; export WAYLAND_DISPLAY=$W; grim /tmp/bukti.png' >> "$LOG" 2>&1
say "grim rc=$? png=$(stat -c %s "$R/tmp/bukti.png" 2>/dev/null) byte"
ls -l "$R/run/user/0" >> "$LOG" 2>&1
say "SELESAI uptime=$(cut -d. -f1 /proc/uptime)s SF=$(pidof surfaceflinger)"
