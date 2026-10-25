#!/system/bin/sh
# Bangun image super ext4 RW (5600MB, ukuran partisi TIDAK diubah)
LOG=/data/local/tmp/mkfs_ext4.log
IMG=/data/local/tmp/artix_super.img
R=/data/data/com.termux/files/usr/var/lib/proot-distro/containers/artix/rootfs/.stage
exec >>$LOG 2>&1

echo "[$(date)] mulai"
rm -f $IMG
truncate -s 5872025600 $IMG
echo "[$(date)] truncate rc=$? size=$(stat -c %s $IMG)"

echo "[$(date)] mke2fs -d (3GB, sabar)"
/system/bin/mke2fs -F -t ext4 -b 4096 -L super -E root_owner=0:0 -d $R $IMG
RC=$?
echo "[$(date)] mke2fs rc=$RC"
if [ $RC -ne 0 ]; then echo "[$(date)] GAGAL"; exit 1; fi

/system/bin/tune2fs -l $IMG | grep -Ei 'volume name|block count|block size|filesystem state|filesystem features|journal'
ls -l $IMG
echo "[$(date)] SELESAI"
