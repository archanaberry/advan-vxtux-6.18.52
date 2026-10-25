#!/system/bin/sh
# Ganti /etc/fstab di image super ext4 (pakai UUID + nofail, tanpa by-name)
P=/data/data/com.termux/files/usr
IMG=/data/local/tmp/artix_super.img
NEW=/data/local/tmp/fstab.new
export LD_LIBRARY_PATH=$P/lib
DB=$P/bin/debugfs
OUT=/data/local/tmp/fstab_patch.log
: > $OUT

echo "=== fstab lama ===" >> $OUT
$DB -R "cat /etc/fstab" $IMG 2>&1 >> $OUT

echo "=== hapus + tulis baru ===" >> $OUT
$DB -w -R "rm /etc/fstab" $IMG >> $OUT 2>&1
$DB -w -R "write $NEW etc/fstab" $IMG >> $OUT 2>&1

echo "=== fstab baru ===" >> $OUT
$DB -R "cat /etc/fstab" $IMG 2>&1 >> $OUT

echo "=== e2fsck ===" >> $OUT
$P/bin/e2fsck -fn $IMG 2>&1 | tail -3 >> $OUT
echo SELESAI >> $OUT
