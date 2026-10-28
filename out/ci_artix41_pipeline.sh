#!/bin/bash
# ci_artix41_pipeline.sh -- FIX: clock tree sebagai MODUL, dimuat dari
# /vxtux/init SETELAH fbcon hidup (artix37).
#
# JAWABAN "kenapa artix34 masuk fbcon, artix35/36 tidak":
#   * artix34: SPRD_COMMON_CLK/UMS512_CLK =m dan modulnya TIDAK PERNAH
#     dimuat -> semua konsumen (eMMC sdio, i2c, PMIC sc2730, ...) dapat
#     -EPROBE_DEFER selamanya -> tidak ada yang probe -> kernel santai
#     jalan sampai device_initcall, lkfb mendaftar, LAYAR HIDUP, init
#     userspace mencetak pesan.
#   * artix35/36: =y -> provider clock terdaftar sejak initcall -> konsumen
#     langsung probe untuk pertama kalinya -> salah satunya menggantung
#     senyap (bus/perangkat belum siap) SEBELUM fbcon -> layar tetap teks
#     LK, tanpa panic, tanpa tulisan cache.  sprd_clk_probe sendiri polos
#     (hanya registrasi + of_clk_add_hw_provider), jadi pelakunya pasti
#     konsumen clock, bukan driver clock-nya.
#
# PERBAIKAN artix37:
#   * enhanced_config.cfg: SPRD_COMMON_CLK=m, SPRD_UMS512_CLK=m (persis
#     nilai artix34 yang terbukti selalu sampai fbcon).
#   * ci_qemu/initramfs/init.c: load_clock_modules() -- finit_module()
#     /vxtux/ko/clk-sprd.ko lalu /vxtux/ko/ums512-clk.ko SETELAH fbcon
#     hidup.  driver_register() memicu driver_deferred_probe_trigger(),
#     jadi konsumen yang tadinya deferred langsung dicoba ulang; kalau ada
#     yang menggantung, gantungnya SETELAH pesan layar tercetak.
#   * pipeline ini menyuntikkan kedua .ko (artefak v17-built-kos run yang
#     SAMA -> vermagic identik) ke initramfs boot.img di /vxtux/ko/.
#     Artefak itu keluar unsigned, jadi enhanced_config.cfg mematikan
#     CONFIG_MODULE_SIG_FORCE (tanpa itu finit_module menolak EKEYREJECTED).
#   * VXTUX_INITTRACE=n: tracer artix36 dimatikan (find_font dari
#     early_param selalu NULL karena font baru terdaftar di device_initcall,
#     jadi tracer tidak pernah menggambar apa pun).  Jalur render yang
#     dipakai = jalur artix34 yang terbukti benar, satu variabel berubah.
#
# KANAL BACA: layar fbcon langsung (dmesg + "VXTUX-MOD:" + "VXTUX-STEP:")
set -u
cd /e/vxtux_build_repo
# artix39 = paket LOKAL.  Kernel/DTB/.ko sudah dibangun runner v17-build;
# pipeline ini hanya memeriksa artefak, mengompilasi ulang /init dari sumber
# saat ini, menyuntikkan modul, lalu memaket boot_artix41.img.
# Tidak butuh jaringan: tanpa gh, tanpa tunggu run.
ART=out/ci_artix41_src
mkdir -p "$ART" out/artix_ramdisk
RUN=$(cat "$ART/RUN.txt" 2>/dev/null || echo local)
echo "[pipeline] artefak lokal: $ART (run=$RUN)"

# --- 0. Image -----------------------------------------------------------
IMG=$(find "$ART" -type f -name "Image" | head -1)
echo "IMG_KETEMU=$IMG"
if [ -z "$IMG" ]; then
  echo "FAILED=no-image (butuh Image dari artefak v17-Image di $ART)"
  echo "        unduh dulu: gh run download <run> -R gryzzlaycutzy121/advan-vxtux-6.18.52 -n v17-Image -D $ART"
  exit 1
fi
cp "$IMG" out/6.18.52-VXTux-UMS512/Image

# --- 0a. konfigurasi efektif build ---------------------------------------
CFG=$(find "$ART" -type f -name "effective.config" | head -1)
echo "CFG=$CFG"
if [ -n "$CFG" ]; then
  for want in CONFIG_SPRD_COMMON_CLK=m CONFIG_SPRD_UMS512_CLK=m \
              CONFIG_REGULATOR_SC2730=m CONFIG_MODULES=y; do
    if grep -qx "$want" "$CFG"; then
      echo "  config OK : $want"
    else
      echo "FAILED=config $want tidak ada di konfigurasi efektif build"
      grep -E "SPRD_(COMMON|UMS512)_CLK|VXTUX_INITTRACE|MODULE_SIG|^CONFIG_MODULES" "$CFG" || true
      exit 1
    fi
  done
  # tracer HARUS mati: simbol tidak boleh =y ("is not set" atau baris hilang)
  if grep -qx "CONFIG_VXTUX_INITTRACE=y" "$CFG"; then
    echo "FAILED=config CONFIG_VXTUX_INITTRACE masih =y (tracer harus mati)"
    exit 1
  fi
  echo "  config OK : CONFIG_VXTUX_INITTRACE tidak =y (tracer mati)"
  # Module signature enforcement is acceptable only for signed modules.
  if grep -qx "CONFIG_MODULE_SIG_FORCE=y" "$CFG"; then
    echo "FAILED=config CONFIG_MODULE_SIG_FORCE masih =y (CI modules built here are unsigned)"
    exit 1
  fi
  echo "  config OK : CONFIG_MODULE_SIG_FORCE tidak =y (CI module package dapat dimuat)"
else
  echo "  (artefak v17-config tidak ada -- verifikasi lewat .ko + Image)"
fi

# --- 0b. cek instrumen di Image -------------------------------------------
echo "[cek] string di Image:"
for s in "vxtux-fbmem" "vxtux-lkfb" "vxtuxfb=" "vxtux-fr"; do
  n=$(grep -c -a -F "$s" out/6.18.52-VXTux-UMS512/Image || true)
  echo "  ada      : $s -> $n"
done
for s in "VXTUX-PROBE" "vxtux-beacon" "VXTUX-ITRACE"; do
  n=$(grep -c -a -F "$s" out/6.18.52-VXTux-UMS512/Image || true)
  echo "  harus 0  : $s -> $n"
done
n=$(grep -c -a -F "VXTUX-ITRACE" out/6.18.52-VXTux-UMS512/Image || true)
[ "$n" -ne 0 ] && { echo "FAILED=tracer initcall masih ikut ke Image (harus mati)"; exit 1; }
# MMC_BLOCK_MINORS limits consecutive minors per disk, not partition count;
# block/partitions/core.c allocates extended minors for later GPT partitions.
if [ -z "$CFG" ]; then
  echo "FAILED=no-config (butuh $ART/**/effective.config dari artefak v17-config)"
  exit 1
fi
# SC2730 WAJIB modul: driver-nya harus terpisah dari Image supaya bisa
# memakai of_match_table sendiri (SC2731 builtin tidak lagi mengklaimnya).
for want in CONFIG_REGULATOR_SC2730=m CONFIG_REGULATOR_SC2731=y; do
  grep -qx "$want" "$CFG" || {
    echo "FAILED=config $want tidak ada di konfigurasi efektif build"
    grep -E 'REGULATOR_SC273' "$CFG" || true
    exit 1
  }
  echo "  config OK : $want"
done
if [ -n "$(grep -x 'CONFIG_REGULATOR_SC2730=y' "$CFG" || true)" ]; then
  echo "FAILED=SC2730 masih builtin (=y); of_match khusus hanya efektif sebagai modul"
  exit 1
fi
grep -qx 'CONFIG_MMC_BLOCK_MINORS=8' "$CFG" || {
  echo "FAILED=unexpected MMC_BLOCK_MINORS; verify the kernel partition-core behavior"
  grep '^CONFIG_MMC_BLOCK_MINORS=' "$CFG" || true
  exit 1
}
echo "  config OK : MMC_BLOCK_MINORS=8 (extended minors cover high GPT partition numbers)"
# clock =m -> provider TIDAK boleh builtin lagi di Image
n=$(grep -c -a -F "sprd,ums512-apapb-gate" out/6.18.52-VXTux-UMS512/Image || true)
echo "  info     : sprd,ums512-apapb-gate di Image -> $n (harus 0, sekarang modul)"
if [ "$n" -ne 0 ]; then echo "FAILED=clock masih builtin di Image padahal harus =m"; exit 1; fi
bad=$(grep -c -a -E "gagal|terdaftar|direservasi|tidak |dengan |untuk " out/6.18.52-VXTux-UMS512/Image || true)
echo "  kata Indonesia di Image -> $bad (harus 0)"
# SC2730 WAJIB modul: kalau compatible-nya tertanam di Image, berarti driver
# regulator SC2730 masih builtin (of_match khusus butuh jadi modul terpisah).
n=$(grep -c -a -F "sprd,sc2730-regulator" out/6.18.52-VXTux-UMS512/Image || true)
echo "  info     : sprd,sc2730-regulator di Image -> $n (harus 0, sekarang modul)"
[ "$n" -ne 0 ] && { echo "FAILED=SC2730 masih builtin di Image (harus =m)"; exit 1; }

# --- 0b2. DTB: rantai suplai eMMC lewat rail SC2730 ----------------------
# artix41 memakai DTB yang dibangun dari DTS saat ini (out/dtb_artix41).
# DTB di artefak v17-dtb berasal dari commit lama dan masih membawa nama rail
# karangan (BUCK_CPU0, ldo-vddemmccore) yang tidak cocok dengan of_match
# driver -> devm_regulator_register() gagal -ENODEV pada rail pertama dan
# SELURUH provider hilang.  Itu akar defer yang tak pernah sembuh.
DTB=out/dtb_artix41/ums512-1h10-vxtux.dtb
if [ ! -f "$DTB" ]; then
  echo "FAILED=no-dtb (bangun dulu: out/build_dtb41.sh)"
  exit 1
fi
echo "DTB=$DTB ($(stat -c%s "$DTB") byte)"
if grep -q -a -F 'sprd,sc2731-regulator' "$DTB"; then
  echo "FAILED=DTB masih memakai compatible sprd,sc2731-regulator"
  exit 1
fi
echo "  dtb OK : tidak ada lagi sprd,sc2731-regulator"
/c/Python*/python.exe out/dtcheck.py "$DTB" || { echo "FAILED=dt-check"; exit 1; }
# Gate nama rail: setiap of_match driver harus punya child DT.  Tanpa ini
# probe regulator berhenti di DCDC_CPU dan semua consumer defer selamanya.
/c/Python*/python.exe out/regcheck.py || { echo "FAILED=regcheck"; exit 1; }
for want in dcdc-cpu ldo-vddemmccore ldo-vddsdcore ldo-vddsdio; do
  if grep -q -a -F "$want" "$DTB"; then
    echo "  dtb OK : rail $want ada"
  else
    echo "FAILED=DTB tidak memuat rail $want (nama node tidak cocok of_match)"
    exit 1
  fi
done

# --- 0c. .ko dari run yang sama (tanda tangan/vermagic cocok) -------------
# Pakai hasil ekstraksi yang sudah ada; hanya untar kalau modulnya belum ada.
if [ ! -f "$ART/kox/drivers/regulator/sc2730-regulator.ko" ]; then
  KOTAR=$(find "$ART" -name 'built_kos.tar.gz' | head -1)
  echo "KOTAR=$KOTAR"
  [ -z "$KOTAR" ] && { echo "FAILED=no-ko-tarball"; exit 1; }
  mkdir -p "$ART/kox"
  tar -xzf "$KOTAR" -C "$ART/kox"
fi
SCKO=$ART/kox/drivers/regulator/sc2730-regulator.ko
[ -f "$SCKO" ] || { echo "FAILED=sc2730-regulator.ko tidak ada di paket kos"; exit 1; }
echo "SCKO=$SCKO ($(stat -c%s "$SCKO") byte)"
# Driver SC2731 tidak boleh lagi mendeklarasikan compatible SC2730: itu yang
# membuat rail eMMC diprogram dengan peta register chip yang salah.
if grep -q -a -F 'of:N*T*Csprd,sc2730-regulator' "$SCKO"; then
  echo "  sc2730-regulator.ko: alias OF SC2730 ADA"
else
  echo "FAILED=sc2730-regulator.ko tidak punya alias OF (modul lama/stale)"
  exit 1
fi
# Semua modul yang dimuat /vxtux/init (late_mods[] di ci_qemu/initramfs/init.c).
# Kalau sintaks ini dan daftar di init.c berbeda, boot akan melaporkan
# "MISSING" di layar -> daftar ini sumber kebenarannya.
LATE_KO_NAMES="clk-sprd ums512-clk sc2730-regulator nvmem-sc27xx-efuse sc27xx_adc rtc-sc27xx gpio-pmic-eic-sprd sc27xx_typec spi-sprd snd-soc-sprd-vbc-v4"
missing=""
for n in $LATE_KO_NAMES; do
  f=$(find "$ART/kox" -name "$n.ko" | head -1)
  if [ -z "$f" ]; then missing="$missing $n"; continue; fi
  cp "$f" out/artix_ramdisk/$n.ko
  printf '  %-26s %s byte' "$n.ko" "$(stat -c%s "$f")"
  if grep -q -a -F '6.18.52-VXTux-UMS512' "$f"; then echo ', vermagic OK'; else echo ", FAILED=vermagic"; exit 1; fi
done
[ -n "$missing" ] && { echo "FAILED=modul tidak ada di paket kos:$missing"; exit 1; }
CLKKO=$(find "$ART/kox" -name 'clk-sprd.ko' | head -1)
UMSKO=$(find "$ART/kox" -name 'ums512-clk.ko' | head -1)
echo "CLKKO=$CLKKO"
echo "UMSKO=$UMSKO"
[ -z "$CLKKO" ] || [ -z "$UMSKO" ] && { echo "FAILED=ko clock tidak ditemukan di tarball"; exit 1; }
for k in "$CLKKO" "$UMSKO"; do
  printf '  %s: %s byte,' "$(basename "$k")" "$(stat -c%s "$k")"
  if grep -q -a -F '~Module signature appended~' "$k"; then
    printf ' signed'
  else
    printf ' unsigned (diterima: MODULE_SIG_FORCE mati)'
  fi
  if grep -q -a -F '6.18.52-VXTux-UMS512' "$k"; then echo ', vermagic OK'; else echo "FAILED=vermagic $k tidak cocok"; exit 1; fi
done
cp "$CLKKO" out/artix_ramdisk/clk-sprd.ko
cp "$UMSKO" out/artix_ramdisk/ums512-clk.ko

# --- 0d. kompilasi init dari SUMBER saat ini (bukan salinan lama di Image) -
# CI menanam init ke dalam Image saat build, tapi Image yang diunduh bisa
# lebih lama daripada ci_qemu/initramfs/init.c.  Init hasil kompilasi lokal
# ini yang disuntikkan ke ramdisk boot.img; karena cpio eksternal di-unpack
# SETELAH initramfs bawaan (init/initramfs.c: do_populate_rootfs) dan file
# yang sama dibuka dengan O_TRUNC, salinan baru selalu menang.
TCHAIN=""
if aarch64-linux-gnu-gcc -static -O2 -march=armv8-a -mabi=lp64 \
     -o out/artix_ramdisk/vxtux-init-new ci_qemu/initramfs/init.c 2>/dev/null; then
    TCHAIN=aarch64-linux-gnu-gcc
else
    # Shim `aarch64-linux-gnu-gcc` di scoop mengarah ke toolchain 13.2 yang
    # sudah dihapus -> "cannot execute 'cc1'".  Pakai binary .exe langsung
    # dari toolchain yang benar-benar ada (15.2.rel1).
    for cand in \
      /c/Users/Administrator/scoop/apps/gcc-aarch64-none-linux-gnu/*/bin/aarch64-none-linux-gnu-gcc.exe \
      /c/Users/Administrator/scoop/apps/gcc-aarch64-none-linux-gnu/*/bin/aarch64-none-linux-gnu-gcc \
      /usr/bin/aarch64-linux-gnu-gcc; do
        [ -x "$cand" ] || continue
        if "$cand" -static -O2 -march=armv8-a -mabi=lp64 \
             -o out/artix_ramdisk/vxtux-init-new ci_qemu/initramfs/init.c \
             2>/dev/null; then
            TCHAIN="$cand"
            break
        fi
    done
fi
[ -n "$TCHAIN" ] || { echo "FAILED=compile-init (cross gcc tidak ada)"; exit 1; }
echo "INIT_LOCAL=$TCHAIN $(stat -c%s out/artix_ramdisk/vxtux-init-new) byte"

# --- 1. initramfs: validasi init + suntik /vxtux/ko/*.ko, lalu gzip ulang -
/c/Python*/python.exe - <<'PYEOF'
import zlib, gzip, sys, struct

img = open('out/6.18.52-VXTux-UMS512/Image', 'rb').read()
assert img[0x38:0x3C] == b'ARM\x64', 'magic ARM64 salah'
print('magic ARM64 OK, ukuran:', len(img))

blob, i = None, 0
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
            blob = img[i:i + consumed]
            break
    except Exception:
        pass
    i += 1
if not blob:
    sys.exit('FAILED: initramfs embedded tidak ditemukan di Image')

p, names, elf_ok, inggris_ok, test_ok, wait_ok, mod_ok, earlylog_ok = 0, [], False, False, False, False, False, False
rawlog_ok, dinit_ok = False, False
fb_gate_ok, sc2730_load_ok = False, False
NEWINIT = open('out/artix_ramdisk/vxtux-init-new', 'rb').read()
init_hdr, init_at, init_old = -1, -1, 0
trailer_at = -1
while p + 110 < len(out) and out[p:p + 6] in (b'070701', b'070702'):
    fsize = int(out[p + 54:p + 62], 16)
    namesize = int(out[p + 94:p + 102], 16)
    name = out[p + 110:p + 110 + namesize - 1].decode('latin1')
    data = p + 110 + namesize
    aligned = (data + 3) & ~3
    if name == 'vxtux/init':
        # Validasi binary yang BARU disuntik, bukan salinan lama di Image.
        init_hdr, init_at, init_old = p, aligned, fsize
        d = NEWINIT
        elf_ok = (d[:4] == b'\x7fELF' and
                  struct.unpack('<H', d[18:20])[0] == 183 and
                  b'chroot OK' in d)
        inggris_ok = (b'1/5 partition list' in d and
                      b'daftar partisi' not in d and
                      b'gagal' not in d and b'akhir' not in d)
        test_ok = (b'VXTUX-WRITE-TEST' in d and
                   b'file write+readback on %s' in d)
        wait_ok = (b'waiting for eMMC' in d and
                   b'storage ready after' in d and b'mknod' in d)
        mod_ok = (b'VXTUX-MOD:' in d and b'/vxtux/ko/clk-sprd.ko' in d and
                  b'/vxtux/ko/ums512-clk.ko' in d)
        # artix40: init harus memuat SEMUA driver =m (gpio eic, adc, rtc,
        # efuse, typec, spi) dan mencetak devices_deferred ke layar.
        defer_ok = (b'devices_deferred' in d and
                    b'/vxtux/ko/gpio-pmic-eic-sprd.ko' in d and
                    b'/vxtux/ko/spi-sprd.ko' in d and
                    b'/vxtux/ko/nvmem-sc27xx-efuse.ko' in d and
                    b'/vxtux/ko/snd-soc-sprd-vbc-v4.ko' in d and
                    b'VXTUX-DEFER' in d)
        earlylog_ok = (b'/cache/boot_log.txt' in d and
                       b'mounted cache and saved early dmesg' in d)
        # Kanal mentah vendor + kandidat init yang benar (dinit-run tidak ada)
        rawlog_ok = (b'VXTUXRS1' in d and
                     b'raw vendor log saved to' in d and
                     b'raw vendor log partition not found' in d)
        dinit_ok = (b'/sbin/init' in d and b'/bin/dinit-run' not in d)
        fb_gate_ok = (b'waiting for fb0/fbcon before late providers' in d and
                      b'fb0 timeout; continue boot' in d)
        sc2730_load_ok = (b'/vxtux/ko/sc2730-regulator.ko' in d and
                          b'loading UMS512 clock modules after fbcon is visible' in d)
        print('vxtux/init: ELF=%s aarch64=%s dinit=%s inggris=%s writetest=%s wait=%s insmod=%s stale-fb-gate=%s sc2730=%s late-mods=%s earlylog=%s rawlog=%s dinitok=%s ukuran=%d'
              % (d[:4] == b'\x7fELF', struct.unpack('<H', d[18:20])[0] == 183,
                 b'chroot OK' in d, inggris_ok, test_ok, wait_ok, mod_ok,
                 fb_gate_ok, sc2730_load_ok, defer_ok, earlylog_ok, rawlog_ok,
                 dinit_ok, fsize))
    if name == 'TRAILER!!!':
        trailer_at = p
        names.append(name)
        break
    names.append(name)
    p = aligned + fsize
    p = (p + 3) & ~3
print('entri:', names)
if 'vxtux/init' not in names:
    sys.exit('FAILED: /vxtux/init tidak ada di initramfs embedded')
if not elf_ok:
    sys.exit('FAILED: initramfs embedded bukan ELF aarch64 dinit yang benar')
if not inggris_ok:
    sys.exit('FAILED: initramfs embedded masih memakai teks Indonesia')
if not test_ok:
    sys.exit('FAILED: initramfs embedded tidak memuat VXTUX-WRITE-TEST')
if not wait_ok:
    sys.exit('FAILED: initramfs embedded tidak memuat wait_for_storage')
if not mod_ok:
    sys.exit('FAILED: initramfs embedded tidak memuat load_clock_modules (VXTUX-MOD)')
if not fb_gate_ok:
    sys.exit('FAILED: init tidak menunggu fb0/fbcon sebelum late provider modules')
if not sc2730_load_ok:
    sys.exit('FAILED: init tidak memuat SC2730 regulator module')
if not defer_ok:
    sys.exit('FAILED: init tidak memuat semua driver =m / tidak mencetak devices_deferred')
if not earlylog_ok:
    sys.exit('FAILED: initramfs embedded tidak memuat early dmesg snapshot ke /cache/boot_log.txt')
if not rawlog_ok:
    sys.exit('FAILED: initramfs embedded tidak memuat kanal log mentah vendor (VXTUXRS1 / common_rs1_a)')
if not dinit_ok:
    sys.exit('FAILED: kandidat init masih memuat /bin/dinit-run yang tidak ada di rootfs Artix')
if init_at < 0:
    sys.exit('FAILED: entri vxtux/init tidak ditemukan untuk disuntik')
if trailer_at < 0:
    sys.exit('FAILED: TRAILER cpio tidak ditemukan')

# --- ganti /vxtux/init dengan binary hasil kompilasi sumber saat ini ------
# Ukuran data berubah, jadi field filesize di header newc ikut ditulis ulang;
# tanpa itu pemindai cpio berikutnya melompat ke offset lama dan rusak.
assert init_old % 4 == len(NEWINIT) % 4, 'alignment cpio tidak cocok'
delta = len(NEWINIT) - init_old
out = bytearray(out)
out[init_at:init_at + init_old] = NEWINIT
out[init_hdr + 54:init_hdr + 62] = ('%08X' % len(NEWINIT)).encode()
out = bytes(out)
trailer_at += delta
print('suntik cpio: vxtux/init (binary lokal) %d byte (delta %+d)' %
      (len(NEWINIT), delta))

# --- suntikkan .ko ke dalam cpio (newc) di /vxtux/ko/ --------------------
def newc_entry(name, data, ino, mode=0o100644):
    namez = name.encode() + b'\0'
    fields = [ino, mode, 0, 0, 1, 0, len(data), 0, 0, 0, 0, len(namez), 0]
    hdr = b'070701' + b''.join(b'%08X' % f for f in fields)
    ent = hdr + namez
    ent += b'\0' * ((4 - len(ent) % 4) % 4)
    ent += data
    ent += b'\0' * ((4 - len(ent) % 4) % 4)
    return ent

# Harus sama persis dengan late_mods[] di ci_qemu/initramfs/init.c.
ko_names = ['clk-sprd', 'ums512-clk', 'sc2730-regulator',
            'nvmem-sc27xx-efuse', 'sc27xx_adc', 'rtc-sc27xx',
            'gpio-pmic-eic-sprd', 'sc27xx_typec', 'spi-sprd',
            'snd-soc-sprd-vbc-v4']
ko_paths = [('vxtux/ko/%s.ko' % n, 'out/artix_ramdisk/%s.ko' % n)
            for n in ko_names]
for arcname, path in ko_paths:
    if not __import__('os').path.isfile(path):
        sys.exit('FAILED: modul wajib hilang: ' + path)
entries = b''
ino = 900001
for arcname, path in ko_paths:
    data = open(path, 'rb').read()
    entries += newc_entry(arcname, data, ino)
    ino += 1
    print('suntik cpio: %-28s %d byte' % (arcname, len(data)))
# Verify the regulator module was built with its DT alias; an older module
# with only alias=platform:... cannot bind the DT regulator child.
sc2730_path = 'out/ci_artix41_src/kox/drivers/regulator/sc2730-regulator.ko'
sc2730 = open(sc2730_path, 'rb').read()
# file2alias emits both "of:N*T*C<compat>" and "of:N*T*C<compat>C*"; the
# plain form is the canonical one, so accept it and print what was found.
if b'alias=of:N*T*Csprd,sc2730-regulator\x00' not in sc2730 and \
   b'alias=of:N*T*Csprd,sc2730-regulator' not in sc2730:
    sys.exit('FAILED: sc2730-regulator.ko lacks the generated SC2730 OF alias')
print('  sc2730-regulator.ko alias OF SC2730: ADA')
# /init -> /vxtux/init (symlink), cadangan bila rdinit tidak dibaca LK.
if 'init' not in names:
    entries += newc_entry('init', b'/vxtux/init', ino, mode=0o120777)
    ino += 1
    names.insert(names.index('vxtux/init'), 'init')
    print('suntik cpio: %-28s -> /vxtux/init' % 'init')
newcpio = out[:trailer_at] + entries + out[trailer_at:]

gz = gzip.compress(newcpio, 9, mtime=0)
open('out/artix_ramdisk/ramdisk', 'wb').write(gz)
print('ramdisk ditulis ulang: out/artix_ramdisk/ramdisk = %d byte (gzip cpio, +%d ko)'
      % (len(gz), len(ko_names)))
PYEOF
if [ $? -ne 0 ]; then echo "FAILED=ramdisk"; exit 1; fi

# --- 1b. verifikasi ramdisk final memuat ko ------------------------------
/c/Python*/python.exe - <<'PYEOF'
import gzip, sys
d = gzip.decompress(open('out/artix_ramdisk/ramdisk', 'rb').read())
NEED_KO = [b'clk-sprd', b'ums512-clk', b'sc2730-regulator',
           b'nvmem-sc27xx-efuse', b'sc27xx_adc', b'rtc-sc27xx',
           b'gpio-pmic-eic-sprd', b'sc27xx_typec', b'spi-sprd',
           b'snd-soc-sprd-vbc-v4']
for need in tuple(b'vxtux/ko/' + n + b'.ko\0' for n in NEED_KO) + \
            (b'vxtux/init\0', b'init\0'):
    ok = need in d
    print('  %-28s %s' % (need[:-1].decode(), 'ADA' if ok else 'HILANG'))
    if not ok:
        sys.exit('FAILED=ramdisk final kehilangan entri')
# Validate /init is a newc symlink to the actual initramfs program.
off = 0
link = None
while off + 110 <= len(d) and d[off:off + 6] in (b'070701', b'070702'):
    mode = int(d[off + 14:off + 22], 16)
    fsize = int(d[off + 54:off + 62], 16)
    namesize = int(d[off + 94:off + 102], 16)
    name = d[off + 110:off + 110 + namesize - 1].decode('latin1')
    start = (off + 110 + namesize + 3) & ~3
    if name == 'init':
        link = (mode, d[start:start + fsize])
    if name == 'TRAILER!!!':
        break
    off = (start + fsize + 3) & ~3
if not link or (link[0] & 0o170000) != 0o120000 or link[1] != b'/vxtux/init':
    sys.exit('FAILED: /init bukan symlink -> /vxtux/init')
print('ramdisk verifikasi OK, /init -> /vxtux/init, isi cpio:', len(d), 'byte')
PYEOF
if [ $? -ne 0 ]; then echo "FAILED=ramdisk-verify"; exit 1; fi

# --- 2. pack boot_artix41.img ---------------------------------------------
/c/Python*/python.exe ci_qemu/mk_boot_artix.py \
  out/6.18.52-VXTux-UMS512/Image \
  out/6.18.52-VXTux-UMS512/boot_artix41.img \
  --base out/base_boot --ramdisk out/artix_ramdisk || { echo "FAILED=pack"; exit 1; }

ls -l out/6.18.52-VXTux-UMS512/boot_artix41.img
md5sum out/6.18.52-VXTux-UMS512/boot_artix41.img
# Proenance untuk skrip .bat: jangan pernah hardcode md5, karena Image dari
# CI berubah setiap commit.  .bat membaca dua berkas ini.
md5sum out/6.18.52-VXTux-UMS512/boot_artix41.img | cut -d' ' -f1 \
  > out/6.18.52-VXTux-UMS512/boot_artix41.md5
stat -c%s out/6.18.52-VXTux-UMS512/boot_artix41.img \
  > out/6.18.52-VXTux-UMS512/boot_artix41.size
md5sum out/dtb_artix41/ums512-1h10-vxtux.dtb | cut -d' ' -f1 \
  > out/dtb_artix41/dtb.md5
stat -c%s out/dtb_artix41/ums512-1h10-vxtux.dtb \
  > out/dtb_artix41/dtb.size
echo "DTB=$(cat out/dtb_artix41/dtb.md5) ($(cat out/dtb_artix41/dtb.size) byte)"
echo "DONE=1"
