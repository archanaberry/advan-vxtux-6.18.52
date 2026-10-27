#!/system/bin/sh
# Verifikasi isi image super ext4 (bukan cuma mkfs sukses)
P=/data/data/com.termux/files/usr
IMG=/data/local/tmp/artix_super.img
export LD_LIBRARY_PATH=$P/lib
OUT=/data/local/tmp/verify_ext4.txt
: > $OUT

echo "=== fstab (root super fisik) ===" >> $OUT
$P/bin/debugfs -R "cat /etc/fstab" $IMG >> $OUT 2>&1

echo "=== /sbin (init dinit) ===" >> $OUT
$P/bin/debugfs -R "ls -l /sbin" $IMG >> $OUT 2>&1

echo "=== /etc/dinit.d ===" >> $OUT
$P/bin/debugfs -R "ls /etc/dinit.d" $IMG >> $OUT 2>&1

echo "=== /usr/bin (adbd, sshd, dinit) ===" >> $OUT
$P/bin/debugfs -R "ls /usr/bin" $IMG 2>/dev/null | tr ' ' '\n' | grep -xE 'dinit|adbd|sshd|bash|agetty|openrc|hyprland|sway|nano|vim' >> $OUT 2>&1

echo "=== /usr/local/bin ===" >> $OUT
$P/bin/debugfs -R "ls /usr/local/bin" $IMG >> $OUT 2>&1

echo "=== superblock ===" >> $OUT
$P/bin/dumpe2fs -h $IMG 2>/dev/null | grep -Ei 'volume name|block count|block size|free blocks|inode count|state|features|journal size' >> $OUT
echo SELESAI >> $OUT
