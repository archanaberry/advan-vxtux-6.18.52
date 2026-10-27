#!/usr/bin/env python3
"""Racik boot.img untuk Artix dari header boot yang sudah terbukti load.

Pakai:
  python mk_boot_artix.py <Image> <boot_a_artix.img> [--base DIR] [--ramdisk DIR]

Isi image:
  header  : header v0 dari boot_a_vxtux_arasan.img (page 4096, addr kernel
            0x80080000, tags 0x86000000) -- dipakai apa adanya.
  kernel  : Image 6.18.52-VXTux-UMS512-artix (storage + DRM built-in).
  ramdisk : kosong (default) -> kernel langsung mount root dari cmdline.
            Bila --ramdisk DIR diberikan, DIR harus punya file 'ramdisk'
            (cpio/gzip) untuk disisipkan.
  cmdline : isi sama dengan CONFIG_CMDLINE di enhanced_config.cfg (lihat di
            bawah). Catatan: LK Unisoc mendeduplikasi cmdline per-kunci dan
            bootargs stok dari DTB selalu didahulukan, jadi root=/init
            dari sini bisa dibuang. Kernel Artix memakai
            CONFIG_CMDLINE_FORCE=y sehingga cmdline bawaan build yang menang;
            baris di bawah dipertahankan sinkron sebagai dokumentasi.

Partisi super TIDAK diubah ukurannya (5600MB); Artix di dalamnya ext4 RW.
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bootimg_tool as bt

# Harus SINKRON dengan CONFIG_CMDLINE di ../enhanced_config.cfg.
CMDLINE = (
    "earlycon console=tty0 console=ttyS1,115200n8 loglevel=8 ignore_loglevel "
    "rdinit=/vxtux/init "
    "loop.max_part=7 printk.devkmsg=on panic=15 "
    "initcall_debug=1 panic_on_warn softlockup_panic=1 hung_task_panic=1 "
    "vxtuxfb=0x9d000000,0x9e000000 "
    "androidboot.console=ttyS1 "
    "androidboot.hardware=ums512_1h10 androidboot.slot_suffix=_a "
    "androidboot.serialno=0123456789ABCDEF androidboot.selinux=permissive"
)
# 2026-10-26: `earlycon` dikembalikan (DT stok: /chosen/stdout-path =
# "serial1:115200n8" -> alias serial1 = /soc/apb@70000000/serial@100000),
# dan `root=PARTLABEL=super rootwait` dibuang: PARTLABEL hanya resolve lewat
# symlink /dev/disk/by-partlabel milik udev, yang tidak ada di initramfs ->
# mount_block_root() menunggu tanpa batas. Root dicari sendiri oleh
# ci_qemu/initramfs/init.c (ext4 >1 GiB) sebelum chroot ke /sbin/init.


def main():
    if len(sys.argv) < 3:
        raise SystemExit(__doc__)
    kernel_path, out = sys.argv[1], sys.argv[2]
    base = "out/base_boot"
    ramdisk_dir = None
    args = sys.argv[3:]
    i = 0
    while i < len(args):
        if args[i] == "--base":
            base = args[i + 1]
            i += 2
        elif args[i] == "--ramdisk":
            ramdisk_dir = args[i + 1]
            i += 2
        else:
            raise SystemExit(f"arg tak dikenal: {args[i]}")

    # Boot image v3/v4 (perangkat ini: header_version 4) TIDAK punya field
    # page_size; ukuran halaman selalu 4096 dan header duduk di offset
    # 0..1584, jadi kernel mulai di offset 4096.  LK menentukan lokasi
    # kernel dari header_size (offset 20), bukan dari page_size.
    page = 4096
    kern = open(kernel_path, "rb").read()
    # ARM64 Image: magic "ARM\x64" di offset 0x38
    magic = struct.unpack("<I", kern[0x38:0x3C])[0] if len(kern) > 0x3C else 0
    if magic != 0x644D5241:
        print(f"PERINGATAN: magic ARM64 Image tidak cocok (0x{magic:08x}) -- "
              f"lanjut, tapi cek kernel-nya!")
    # --- RELAY 2MB (2026-10-26, bedah out/logs/artix13) -------------------
    # LK Unisoc TIDAK membaca text_offset dari Image; ia selalu memuat kernel
    # ke 0x80080000 (konstanta tetap, log: "[start_linux] start linux at
    # PC:0x80080000, dt_addr:0x9df00000").  Header Image kernel modern
    # (>=5.x) berisi text_offset = 0, dan
    # Documentation/arch/arm64/booting.rst:138 mewajibkan:
    #   "The Image must be placed text_offset bytes from a 2MB aligned base
    #    address anywhere in usable system RAM and called there."
    # Dengan text_offset 0, Image HARUS mendarat di alamat 2MB-aligned.
    # 0x80080000 = 0x80000000 + 0x80000 -> BUKAN 2MB-aligned, jadi pemetaan
    # awal/MMU salah 512KB dan kernel mati senyap: tidak ada satu baris pun
    # di layar maupun partisi cache (bukti: kernel_size kita 42209792 dibaca
    # benar 19x, nol jejak 6.18.52 di cache.bin).  Kernel stok 5.4 aman
    # karena Image-nya ber-text_offset 0x80000 (0x80000000 + 0x80000).
    #
    # Perbaikan tanpa menyentuh LK: awali blob kernel dengan satu instruksi
    # `b`.  LK menyalin blob ke 0x80080000, relay melompat ke 0x80200000, dan
    # Image asli ditaruh RELAY_OFF di dalam blob sehingga mendarat tepat di
    # 0x80200000 (2MB-aligned).  x0 (DTB) dan seluruh register tidak diubah.
    LK_LOAD = 0x80080000
    RELAY_AT = 0x80200000
    relay_off = RELAY_AT - LK_LOAD              # 0x180000 (1.5 MiB, page-aligned)

    # --- STUB CAT FRAMEBUFFER (2026-10-26, bedah out/logs/artix20) ----------
    # Bukti dari uboot_log artix20:
    #   [start_linux] start linux at PC:0x80080000, dt_addr:0x9df00000
    #   lk consume time: 14324ms
    #   -> lalu "panic type boot count 473" (watchdog) tanpa satu baris
    #      kernel 6.18 punya di cache.bin (cache.bin hanya berisi dmesg
    #      stok 5.4.210, ditulis pstore_blk ke /dev/mmcblk0p49 -- yang
    #      berarti eMMC tidak pernah hidup).
    # Artinya LK MEMANG memanggil kernel kita, lalu kernel mati senyap
    # sebelum bisa mencetak apa pun.  Karena UART tidak dirutekan ke COM
    # dan fbcon baru aktif jauh di kemudian hari, satu-satunya kanal
    # AWAL yang pasti terlihat adalah framebuffer LK itu sendiri
    # (lcd_base=0x9e000000, DISPC terus memindai dari situ).
    #
    # Maka: sebelum melompat ke kernel, stub perintah murni (MMU masih
    # mati, x0 = DTB tidak disentuh) mengecat 8 MiB pertama framebuffer
    # dengan MAGENTA.  Kalau layar berubah magenta -> serah-terima LK
    # berhasil dan kode kita di 0x80080000 benar-benar berjalan.  Kalau
    # layar tetap di logo -> LK tidak pernah melompat / blob tidak ada
    # di tempat yang kita kira.
    #
    # Perataan: panel 1200x2000 @32bpp, stride 4800 (fb stok 0x9E4000 =
    # 1200*2160*4).  0x200000 word = 8 MiB ~= 1747 baris -> masih di dalam
    # buffer, tidak menabrak apa pun.
    FB_BASE = 0x9E000000
    # --- UJI 4 PITA (2026-10-27, bedah foto layar BIRU artix21) -------------
    # Foto: layar biru SOLID penuh, tanpa sisa logo, tanpa kotak beacon,
    # tanpa strip merah panic.  Ini justru BUKTI serah-terima LK -> blob
    # kita BERHASIL: layar berubah dari logo menjadi biru, padahal delta
    # satu-satunya dari artix20 (logo diam) adalah stub cat fb ini.
    #
    # Mengapa biru, bukan magenta 0x00FF00FF?  Dalam urutan RGB/BGR/ARGB
    # apa pun, byte {FF,00,FF,00} selalu memberi R=B=FF -> magenta; biru
    # murni butuh R=0 DAN G=0, mustahil dari word itu.  Jadi biru BUKAN
    # masalah urutan kanal XR24/XB24.  Dua kandidat nyata:
    #   (a) byte paling atas 0x00 = alpha 0 -> piksel tembus pandang,
    #       yang terlihat warna latar/lapisan dasar DISPC (biru);
    #   (b) pipeline DPU tidak memindai buffer ini lagi (kernel/DPU
    #       ambil alih) -> biru = warna latar DPU, bukan tulisan kita.
    #
    # Maka sekarang: cat SELURUH layar (4 x 600.000 word = 9,6 MB,
    # muat di fb 0x9E4000) dengan ALPHA 0xFF dan empat pita yang kanal
    # merah/birunya saling bertukar posisi.  Cara membaca hasilnya:
    #   pita1 MERAH, pita3 BIRU  -> XRGB8888 (XR24); biru tadi = alpha
    #   pita1 BIRU,  pita3 MERAH -> XBGR8888 (XB24)
    #   biru solid lagi           -> bukan urutan kanal; DPU/pipeline
    #                                (kemungkinan b), lanjut ke bedah DRM
    # --- HASIL ARTIX22 + RISET WEB (2026-10-27): DPU baca RGB565! ----------
    # Foto artix22: pita abu-abu + kuning.  Kalau tiap word 32-bit dibaca
    # sebagai DUA piksel RGB565 (little-endian), semuanya cocok SEMPURNA:
    #   0x00FF00FF (artix21) -> 0x00FF,0x00FF = biru padat   <- layar biru
    #   0xFFFF0000 -> 0x0000,0xFFFF = hitam+putih = ABU-ABU <- pita 1
    #   0xFF00FF00 -> 0xFF00,0xFF00 = KUNING                 <- pita 2
    # Riset web memastikan DPU Unisoc (dpu_r2p0/r4p0) memang punya mode
    # RGB565/BGR565 di samping XRGB8888; "fb size 0x9e4000" hanya bukti
    # alokasi 4 B/piksel, BUKAN bukti format scanout (referensi:
    # patchew.org/20211207142717.30296-5, github.com/strongtz/linux-sprd
    # drivers/gpu/drm/sprd/dpu/).  Jadi asumsi XRGB8888 di seluruh kode
    # (vxtux_lkfb.c, beacon) salah dasar -- MEMBERIKAN SOLUSI: semua warna
    # ditulis dobel sebagai dua halfword identik.
    #
    # Pita RGB565 murni: merah 0xF800, hijau 0x07E0, biru 0x001F, putih 0xFFFF.
    # Geometri (koreksi artix24): 1 word = 4 byte, jadi 600.000 word =
    # 2,4 MB -- muatnya cuma 2 pita di layar RGB565 4,8 MB, itulah sebabnya
    # artix23 hanya menampilkan MERAH+HIJAU (biru & putih keluar layar).
    # Pita yang benar: 4 x 300.000 word = 4 x 1,2 MB = 4,8 MB =
    # 1200*2000*2 -> pas 4 pita selebar 1/4 layar, urut penuh.
    # --- ARTIX26: STUB DIAGNOSTIK (2026-10-27, sesudah artix25) ------------
    # artix25 (pita merah/hijau/biru/putih, alpha FF) tetap menghasilkan
    # PELANGI yang sama: 4 pita benar, jadi serah-terima LK -> stub -> `br
    # 0x80200000` berjalan, TAPI kotak putih beacon (early_initcall di
    # vxtux_beacon.c) tidak pernah muncul, dan cache.bin tidak memuat satu
    # jejak pun 6.18 (tidak ada "vxtux-beacon:", tidak ada oops).
    # Terbukti juga string di Image: "vxtux-beacon", "vxtux-lkfb",
    # "vxtuxfb=" semuanya ADA di Image yang di-flash, jadi instrumennya ikut
    # ter-build -- bukan driver yang hilang.
    #
    # Artinya kernel mati SEBELUM initcall pertama, yaitu di head.S /
    # early_map_kernel() / setup_arch() -- di mana tidak ada konsol sama
    # sekali.  Diagnostik harus dilakukan SEBELUM melompat, karena stub-lah
    # satu-satunya kode kita yang pasti berjalan.
    #
    # Tersangka utama: LK Unisoc menyerahkan kendali dengan MMU + D-cache
    # HIDUP.  Jalur boot 6.x untuk kasus itu (early_map_kernel() ->
    # map_kernel() -> idmap_cpu_replace_ttbr1) SAMA SEKALI tidak pernah diuji
    # di QEMU: qemu-boot.yml selalu `-kernel` langsung, dan QEMU selalu
    # melompat dengan MMU mati.
    #
    # Stub ini mengubah 4 pita dari "warna uji" menjadi "jawaban":
    #   pita 1 (atas)  MERAH = SCTLR_EL1.M mati (normal)
    #                  HIJAU = SCTLR_EL1.M hidup  <-- tersangka
    #   pita 2         BIRU  = CurrentEL = EL1
    #                  PUTIH = EL2, MAGENTA = EL3
    #   pita 3         HIJAU = magic FDT 0xD00DFEED valid di [x0]
    #                  MERAH = tidak valid (DTB tidak sampai ke kernel)
    #   pita 4         HIJAU = SCTLR_EL1.C (D-cache) hidup
    #                  MERAH = D-cache mati
    # 1 word 32-bit = 2 piksel RGB565, 1 baris = 1200 px = 2400 byte = 600 word.
    # Chip jawaban = 50 baris = 30.000 word; sisanya dicat HITAM supaya kotak
    # putih dari dalam kernel (init/main.c, vxtux_beacon.c) menonjol.
    FB_ROW_WORDS = 600
    CHIP_WORDS = 50 * FB_ROW_WORDS      # 30.000 word = 50 baris
    TOTAL_WORDS = 2000 * FB_ROW_WORDS   # 1.200.000 word = 1200x2000 RGB565
    REST_WORDS = TOTAL_WORDS - 4 * CHIP_WORDS
    BAND_COLORS = {
        'red':     0xF800F800,             # MERAH  (565 dobel)
        'green':   0x07E007E0,             # HIJAU
        'blue':    0x001F001F,             # BIRU
        'white':   0xFFFFFFFF,             # PUTIH
        'magenta': 0xF81FF81F,             # MAGENTA (R+B)
        'black':   0x00000000,             # HITAM
    }
    STUB_OFF = 0x40                         # sesudah header Image (64 byte)

    # --- SELF-TEST DIMATIKAN (2026-10-27, setelah ujian lulus) --------------
    # Uji 4-pita (MMU / CurrentEL / magic FDT / D-cache) sudah terbukti di
    # foto artix27: kernel boot penuh sampai fbcon + initcall.  Stub kini
    # cuma melewati: lompat lurus ke Image asli di 0x80200000 tanpa menyapu
    # framebuffer, supaya layar bersih dan fbcon yang menggambar (via
    # vxtux_lkfb) langsung terlihat tanpa tumpukan kotak warna.
    def _passthrough_stub():
        """stub minimal: x1=FB_BASE disentuh tidak, hanya br ke RELAY_AT."""
        asm = _Asm()
        asm.words(_mov_imm(2, RELAY_AT, 1))   # x2 = 0x80200000
        asm.words([0xD61F0040])               # br x2
        return asm.assemble()

    def _movz(rd, imm16, sf):
        return ((0xD2800000 if sf else 0x52800000) |
                ((imm16 & 0xFFFF) << 5) | rd)

    def _movk(rd, imm16, hw, sf):
        return ((0xF2800000 if sf else 0x72800000) | (hw << 21) |
                ((imm16 & 0xFFFF) << 5) | rd)

    def _mov_imm(rd, val, sf):
        """movz/movk untuk membangun konstanta 32-bit."""
        insns = [_movz(rd, val & 0xFFFF, sf)]
        hi = (val >> 16) & 0xFFFF
        if hi:
            insns.append(_movk(rd, hi, 1, sf))
        return insns

    class _Asm:
        """Assembler mini: cukup untuk b/bl/b.ne/tbz/tbnz berlabel."""

        def __init__(self):
            self.items = []

        def words(self, insns):
            for w in insns:
                self.items.append(('insn', w))

        def label(self, name):
            self.items.append(('label', name))

        def b(self, name):
            self.items.append(('br', 0x14000000, name, 26))

        def bl(self, name):
            self.items.append(('br', 0x94000000, name, 26))

        def bne(self, name):
            self.items.append(('br', 0x54000001, name, 19))

        def tbnz(self, rt, bit, name):
            self.items.append(('tbit', rt, bit, name, 1))

        def assemble(self):
            pos, idx = {}, 0
            for it in self.items:
                if it[0] == 'label':
                    pos[it[1]] = idx
                else:
                    idx += 1
            out = []
            for it in self.items:
                if it[0] == 'label':
                    continue
                if it[0] == 'insn':
                    out.append(it[1])
                    continue
                if it[0] == 'br':
                    base, name, bits = it[1], it[2], it[3]
                    off = pos[name] - len(out)
                    if bits == 26:
                        out.append(base | (off & 0x3FFFFFF))
                    else:
                        out.append(base | ((off & 0x7FFFF) << 5))
                    continue
                _, rt, bit, name, is_tbnz = it
                off = pos[name] - len(out)
                base = 0x37000000 if is_tbnz else 0x36000000
                out.append(base | ((bit >> 5) << 31) | ((bit & 0x1F) << 19) |
                           ((off & 0x3FFF) << 5) | rt)
            return out

    A = _Asm()
    A.words(_mov_imm(1, FB_BASE & 0xFFFFFFFF, 1))   # x1 = kursor framebuffer

    def _band(color, words=CHIP_WORDS):
        A.words(_mov_imm(2, BAND_COLORS[color], 0))  # w2 = warna
        A.words(_mov_imm(3, words, 0))               # w3 = jumlah word
        A.bl('paint')

    A.words([0xD5381009])                    # mrs  x9, sctlr_el1
    A.tbnz(9, 0, 'mmu_on')
    _band('red')                             # pita1: MMU mati
    A.b('b2')
    A.label('mmu_on')
    _band('green')                           # pita1: MMU HIDUP
    A.label('b2')

    A.words([0xD538424A])                    # mrs  x10, CurrentEL
    A.tbnz(10, 3, 'el23')
    _band('blue')                            # pita2: EL1
    A.b('b3')
    A.label('el23')
    A.tbnz(10, 2, 'el3')
    _band('white')                           # pita2: EL2
    A.b('b3')
    A.label('el3')
    _band('magenta')                         # pita2: EL3
    A.label('b3')

    A.words([0xB940000B])                    # ldr  w11, [x0]  (x0 = DTB)
    # FDT header big-endian: byte di memori D0 0D FE ED, jadi ldr 32-bit
    # little-endian menghasilkan 0xEDFE0DD0 (bukan 0xD00DFEED!).  Inilah
    # sebab pita 3 artix26 merah: pemeriksaannya sendiri yang salah, bukan
    # DTB-nya.
    A.words(_mov_imm(12, 0xEDFE0DD0, 0))     # w12 = magic FDT (LE view)
    A.words([0x6B0C017F])                    # cmp  w11, w12  (rn=11, rm=12)
    A.bne('fdt_bad')
    _band('green')                           # pita3: DTB valid
    A.b('b4')
    A.label('fdt_bad')
    _band('red')                             # pita3: DTB TIDAK valid
    A.label('b4')

    A.tbnz(9, 2, 'dcache_on')
    _band('red')                             # pita4: D-cache mati
    A.b('go')
    A.label('dcache_on')
    _band('green')                           # pita4: D-cache hidup
    A.label('go')

    _band('black', REST_WORDS)                # sisanya hitam (latar kotak kernel)

    A.words(_mov_imm(2, RELAY_AT, 1))         # x2 = 0x80200000
    A.words([0xD61F0040])                     # br x2

    A.label('paint')
    A.words([0xB9000022])                     # str  w2, [x1]
    A.words([0x91001021])                     # add  x1, x1, #4
    A.words([0x71000463])                     # subs w3, w3, #1
    A.bne('paint')
    A.words([0xD65F03C0])                     # ret

    # Self-test dimatikan: pakai stub pass-through (tanpa pita warna).
    # Aktifkan kembali dengan mengembalikan `stub = A.assemble()` di bawah.
    _ = A  # assembler lama dipertahankan utk referensi; tidak dipakai.
    stub = _passthrough_stub()
    # stub = A.assemble()   # <-- uncomment utk mengaktifkan lagi uji 4-pita

    # Stub kini cuma 2 instruksi (8 B) -- taruh tepat setelah header (0x40)
    # dan arahkan cabang header ke sana (bukan 0x40 yang kini di-padding).
    STUB_REAL_OFF = 0x40
    relay_hdr = bytearray(kern[:64])
    # word0: b ke stub (offset 0x40 -> imm26 = 0x10); x0 (DTB) tidak disentuh.
    struct.pack_into("<I", relay_hdr, 0,
                     0x14000000 | ((STUB_REAL_OFF >> 2) & 0x03FFFFFF))
    struct.pack_into("<I", relay_hdr, 4, 0xD503201F)          # nop (pengaman)
    struct.pack_into("<Q", relay_hdr, 8, 0)                   # text_offset
    struct.pack_into("<Q", relay_hdr, 16, relay_off + len(kern))  # image_size
    struct.pack_into("<Q", relay_hdr, 24, 0xA)                # flags
    struct.pack_into("<I", relay_hdr, 56, 0x644D5241)         # "ARM\x64"
    relay_hdr += b"\0" * (STUB_OFF - len(relay_hdr))
    relay_hdr += b"".join(struct.pack("<I", w) for w in stub)
    assert len(relay_hdr) < relay_off
    kern = bytes(relay_hdr) + b"\0" * (relay_off - len(relay_hdr)) + kern
    print(f"relay: blob mulai `b 0x{LK_LOAD + STUB_REAL_OFF:x}` (stub pass-through) di "
          f"0x{LK_LOAD:x}, lalu `br` ke 0x{RELAY_AT:x}; "
          f"Image asli di blob+0x{relay_off:x}")
    print(f"stub: self-test DIMATIKAN, lompat lurus ke kernel "
          f"(tanpa pita warna di 0x{FB_BASE:x})")

    rd = b""
    if ramdisk_dir:
        p = os.path.join(ramdisk_dir, "ramdisk")
        rd = open(p, "rb").read()
        print(f"ramdisk: {p} ({len(rd)} byte)")
    if not rd:
        # LK Unisoc v4: ramdisk_size = 0 di offset 12 -> LK mencari ramdisk
        # di partisi init_boot.  Partisi init_boot perangkat ini TIDAK
        # berisi boot image v4 yang valid -> "bad init boot image header,
        # give up boot!" -> thread kernel_dtb_parse panic -> bootloop tanpa
        # pernah menjalankan kernel (bukti: uboot_log artix6/7, 21 panic
        # beruntun "parser init boot image header_v4 fail").  Karena itu
        # ramdisk WAJIB ada; pakai --ramdisk out/artix_ramdisk (init
        # diagnostik /vxtux/init, hasil compile ci_qemu/initramfs/init.c).
        raise SystemExit(
            "ramdisk kosong -- LK akan panic parse init_boot (bootloop). "
            "Berikan --ramdisk out/artix_ramdisk")

    # --- HEADER v4 disintesis dari nol (bedah boot_a.bin stok, 2026-10-26) --
    # Dump backup stok (backupan-ori-advantabvxlite_t1.1/backup/boot_a.bin):
    #   0000: 41 4e 44 52 4f 49 44 21   "ANDROID!"
    #   0008: 00 22 45 02               kernel_size  = 38085120
    #   000c: 88 6d 16 00               ramdisk_size = 1469832
    #   0010: 76 01 00 1a               os_version   = 0x1a000176
    #   0014: 30 06 00 00               header_size  = 1584   <<-- KUNCI
    #   0018..0027: 00                  reserved[4]
    #   0028: 04 00 00 00               header_version = 4
    #   002c.. : 00                     cmdline[1536] KOSONG (stok memakai
    #                                   /chosen/bootargs dari DTB)
    #
    # Versi lama menyalin header v0 dari out/base_boot/header.bin, sehingga
    # offset 16/20/32/36 berisi sisa field v0 yang salah arti bagi v4:
    #   offset 16 = 293467 (ramdisk_size disalin dua kali -> os_version sampah)
    #   offset 20 = 0x84000000 (tags_addr lama -> jadi header_size!)
    #   offset 32 = 0x86000000, offset 36 = 4096 (reserved menyalah)
    # Stok punya header_size = 1584 = sizeof(boot_img_hdr_v3), sehingga LK
    # membaca kernel dari offset 4096.  Dengan 0x84000000 LK menghitung
    # offset kernel 2,2 GB -- di luar partisi boot_a (64 MB) -- dan yang
    # berakhir di 0x80080000 bukan kernel kita.  Itulah sebabnya tidak ada
    # satu instruksi pun yang pernah berjalan: layar diam di logo LK,
    # partisi cache tak tersentuh, walau LK mencetak
    # "boot_a kernel read OK, page size = 0x29c2000, locate to 0x80080000".
    V4_HEADER_SIZE = 1584
    buf = bytearray(V4_HEADER_SIZE)
    buf[0:8] = b"ANDROID!"
    struct.pack_into("<I", buf, 8, len(kern))          # kernel_size
    struct.pack_into("<I", buf, 12, len(rd))           # ramdisk_size
    struct.pack_into("<I", buf, 16, 0)                 # os_version
    struct.pack_into("<I", buf, 20, V4_HEADER_SIZE)    # header_size -> 1584
    struct.pack_into("<I", buf, 40, 4)                 # header_version = 4
    buf[48:64] = b"vxtux-arasan\0\0\0\0"               # name (v0, tak dipakai v4)
    # Cmdline ditaruh di offset 64 karena bukti uboot_log menunjukkan LK
    # membacanya di sana, bukan di offset 44 milik v4:
    #   "android kernel cmdline :earlycon console=tty0 console=ttyS1..."
    # Offset 44..64 sengaja dibiarkan nol supaya bila LK memakai field
    # cmdline v4 ia melihat string kosong -- persis seperti stok.  Kernel
    # sendiri memakai CONFIG_CMDLINE_FORCE=y, jadi cmdline image ini hanya
    # berpengaruh pada LK.
    buf[64:576] = CMDLINE.encode()[:511].ljust(512, b"\0")
    struct.pack_into("<I", buf, 1580, 0)               # signature_size (v4)

    outb = bytearray(buf)
    outb.extend(b"\0" * (bt.align(len(buf), page) - len(buf)))

    def add(blob):
        if not blob:
            return
        # Setiap komponen (header, kernel, ramdisk) harus mulai tepat di
        # batas page -- LK membaca kernel dari offset page_size (4096).
        # Tanpa padding INI, kernel menempel di offset 0x660 dan LK memuat
        # kernel geser 0x9A0 byte -> entry mendarat di pad NOP -> bootloop
        # (bukti: ELR 0x80177288 = brk #0x800 di uboot_log, 2026-10-25).
        outb.extend(b"\0" * (bt.align(len(outb), page) - len(outb)))
        outb.extend(blob)
        outb.extend(b"\0" * (bt.align(len(blob), page) - len(blob)))

    add(kern)
    add(rd)
    assert len(outb) % page == 0
    os.makedirs(os.path.dirname(out) or ".", exist_ok=True)
    open(out, "wb").write(bytes(outb))
    print(f"ditulis {out}: {len(outb)} byte "
          f"(kernel={len(kern)}, ramdisk={len(rd)}, page={page})")
    print(f"cmdline: {CMDLINE}")


if __name__ == "__main__":
    main()
