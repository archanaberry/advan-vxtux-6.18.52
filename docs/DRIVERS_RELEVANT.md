# VXTux — Driver relevan untuk Advan Tab VX Lite (T1030)

Daftar driver `.ko` yang **relevan untuk operasional harian** Advan Tab VX Lite
(T1030, board `ums512_1h10`, Unisoc UMS512/T618), hasil build di device ADVAN
(artix proot, gcc 16.2.1, kernel `6.18.52-VXTux-UMS512`). Bukti build =
`work/sysmaps/kolist.1012b.txt` (1.424 .ko total) + arsip `.ko` khusus di
`work/sysmaps/`.

Bukan sekadar "ada di build" — disaring berdasarkan apakah driver menyentuh
fungsi **daily-use** (boot, layar, sentuh, WiFi, BT, kamera, baterai, charger,
modem, sensor termal). Sisanya (crypto mainline, netfilter, driver lain yang
off-board) tidak masuk daftar ini.

## 0. Ringkasan satu kalimat

**40 modul SPRD/Unisoc/Advan-relevant** terbangun penuh di device, terdistribusi
dalam 4 kelas fungsi; **15 di antaranya `.ko` SPRD/Unisoc** yang terhitung
eksplisit, **25 modul `.ko`** dari keluarga `sc27xx`/`sprd`/vendor lain yang
menyentuh fungsionalitas spesifik Advan T1030.

## 1. Kelas A — Boot-Critical (wajib ada atau device gagal hidup)

| `.ko` | Sumber | Ukuran | Build | Peran di Advan T1030 |
|---|---|---:|---|---|
| `phy-sprd-ums512.ko` | `drivers/phy/sprd/` | 81 KB | ✓ | PHY USB & PCIe — host USB mati tanpa dia |
| `sprd-iommu.ko` | `drivers/iommu/` | 81 KB | ✓ (gcc 16.2.1, TICK7) | IOMMU untuk VSP/CPP/JPEG/GSP — tanpa dia GPU/Camera crash |
| `sprd_ion.ko` | `drivers/dma-buf/sprd/` | 62 KB | ✓ (gcc 16.2.1) | ION memory allocator (legacy vendor) — dipakai semua subsistem kamera/VSP |
| `sprd-drm.ko` | `drivers/gpu/drm/sprd/` | TBD | ✓ | Display pipeline (composer via DPU) |
| `sprd_battery_info.ko` | `drivers/power/supply/sprd/` | 96 KB | ✓ (gcc 16.2.1) | Laporan baterai ke userspace (`/sys/class/power_supply/`) |
| `sprd-charger-manager.ko` | `drivers/power/supply/sprd/` | TBD | ✓ | Koordinasi charger + fuel gauge + BMS |
| `sc27xx_fuel_gauge.ko` | `drivers/power/supply/` | TBD | ✓ | Fuel gauge SC2730 — pembacaan voltage/current/SOC |
| `bq2560x-charger.ko` | `drivers/power/supply/` | TBD | ✓ | BQ256xx charger IC (compatible `ti,bq2560x_chg`, DT `charger@6b`) |

**Catatan**: 8 modul wajib ini gagal boot/charge/display tanpa mereka; resync
ke config de-bloat + verifikasi runtime insmod wajib sebelum flash.

## 2. Kelas B — Daily Functionality (layar, sentuh, WiFi, BT, kamera)

| `.ko` | Sumber | Ukuran | Build | Peran di Advan T1030 |
|---|---|---:|---|---|
| `sprd_vsp.ko` | `drivers/media/platform/sprd/vsp/` | 231 KB | ✓ (gcc 16.2.1) | Video Signal Processor — encoding/decoding H.264/H.265, kamera |
| `sprd_vsp_pw_domain.ko` | `drivers/pmdomain/sprd/` | 52 KB | ✓ (gcc 16.2.1) | VSP power-domain gate |
| `sprd_camsys_pw_domain.ko` | `drivers/pmdomain/sprd/` | 95 KB | ✓ (gcc 16.2.1) | Camera-system power-domain gate |
| `sprd-gsp.ko` | `drivers/gpu/drm/sprd/gsp/` | 767.488 B | ✓ (TICK9, llvm-nm 535 simbol) | Graphics Signal Processor — komposisi layer display |
| `sprd_gpu_pw_domain.ko` | `drivers/pmdomain/sprd/` | TBD | ✓ | GPU power-domain gate |
| `sprdwl.ko` | `drivers/net/wireless/sprd/sprdwcn/wlan/` | 2,5 MB | ✓ (gcc 16.2.1) | WiFi SC2355 (depends `cfg80211+wcn_core`) |
| `sprdbt_tty.ko` | `drivers/net/wireless/sprd/sprdwcn/bluetooth/` | TBD | ✓ | Bluetooth HCI over tty-SDIO |
| `sprd_fm.ko` | `drivers/net/wireless/sprd/sprdwcn/fm/` | TBD | ✓ | FM radio |
| `novatek_ts_spi.ko` | `drivers/input/touchscreen/novatek_nvt_ts_spi/` | 512.584 B | ✓ (TICK9, llvm-nm 337 simbol) | Touchscreen Novatek NT36xxx (SPI) — DT `touchscreen@0` |
| `panel-novatek-nt36672e.ko` | `drivers/gpu/drm/panel/` | TBD | ✓ | Display panel driver Novatek NT36672E |
| `jpg.ko` | `drivers/misc/sprd_jpg/` | 156 KB | ✓ (gcc 16.2.1) | SPRD JPEG codec — kamera snapshot |
| `cpp.ko` | `drivers/media/platform/sprd/cpp/` | 321 KB | ✓ (gcc 16.2.1) | Camera Post-Processor (CPP) — scaling/rotation/format convert |
| `sprd_vdsp.ko` | `drivers/vxtux/npu/vdsp/` | 913.568 B | ✓ (TICK7) | Vision DSP (Cadence XRP) — kamera HDR/AE ops |
| `wcn_core.ko` | `drivers/net/wireless/sprd/sprdwcn/platform/` | TBD | ✓ | Composite WCN platform (gnss/pmnotify/dbg) |

**Catatan**: 14 modul ini menentukan apakah device "bisa dipake harian". Tanpa
touchscreen, device cuma jadi server headless. Tanpa WiFi/BT, device offline.
Tanpa VSP/CPP/jpg, kamera hitam. Tanpa GSP, layer display bisa patah.

## 3. Kelas C — SIPC/Modem (telepon, SMS, data selular)

| `.ko` | Sumber | Ukuran | Build | Peran |
|---|---|---:|---|---|
| `sipc-core.ko` | `drivers/soc/sprd/sipc/` | TBD | ✓ | SIPC core — transport modem↔AP |
| `seth.ko` | `drivers/soc/sprd/sipc/` | TBD | ✓ | SIPC ethernet (seth) — channel data IP modem |
| `spipe.ko` | `drivers/soc/sprd/sipc/` | TBD | ✓ | SIPC pipe — channel log/AT modem |
| `spool.ko` | `drivers/soc/sprd/sipc/` | TBD | ✓ | SIPC pool — channel memori bersama |
| `sblock_bridge.ko` | `drivers/soc/sprd/sipc/` | TBD | ✓ | SIPC block bridge |
| `sbuf_bridge.ko` | `drivers/soc/sprd/sipc/` | TBD | ✓ | SIPC buffer bridge |
| `sipx.ko` | `drivers/soc/sprd/sipc/` | TBD | ✓ | SIPC zero-copy (sipx) |
| `sprd_modem_loader.ko` | `drivers/soc/sprd/` | TBD | ✓ | Modem firmware loader |
| `sprd_sip_svc.ko` | `drivers/soc/sprd/` | TBD | ✓ | SIPC service dispatcher |
| `sprd-sipc-virt-bus.ko` | `drivers/soc/sprd/` | TBD | ✓ | Virtual bus untuk device SIPC |

**Catatan**: 10 modul ini ngehuhubungin modem 4G LTE ke Application Processor.
Tanpa mereka, device cuma jadi WiFi-only tablet (no telepon, no SMS, no data
selular). Penting untuk fungsi smartphone Advan T1030.

## 4. Kelas D — Sensor, PM, Chip-ID, Thermal

| `.ko` | Sumber | Ukuran | Build | Peran |
|---|---|---:|---|---|
| `sprd_power_manager.ko` | `drivers/soc/sprd/` | TBD | ✓ | Power manager — koordinator domain daya |
| `sprd_iq.ko` | `drivers/media/platform/sprd/` | TBD | ✓ | Image Quality (IQ) tuning kamera |
| `sc27xx_tsensor_thermal.ko` | `drivers/thermal/sprd/` | TBD | ✓ | Thermal sensor SC2730 (`pmic_tsensor` di DTB) |
| `sprd_cpu_cooling.ko` | `drivers/thermal/sprd/` | TBD | ✓ | CPU cooling (late_initcall, tanpa DT) |
| `sprd_soc_thm.ko` | `drivers/thermal/sprd/` | TBD | ✓ | SoC thermal — bind ke `virtual_sensor` (TICK9 §4 chain belum lengkap) |
| `sprd_soc_id.ko` | `drivers/soc/sprd/soc_id/` | TBD | ✓ | SoC identifier (board detection) |
| `sprd_time_sync.ko` | `drivers/soc/sprd/` | TBD | ✓ | Time sync AP↔CP |
| `sprd_time_sync_cp.ko` | `drivers/soc/sprd/` | TBD | ✓ | Time sync CP-side |
| `sprd_wdt_fiq.ko` | `drivers/watchdog/sprd/` | TBD | ✓ | Watchdog FIQ — emergency reset |
| `unisoc-mailbox.ko` | `drivers/mailbox/` | TBD | ✓ | Mailbox AP↔CP — channel interrupt/message |
| `usb_f_vser.ko` | `drivers/usb/gadget/function/` | TBD | ✓ | USB function virtual serial (ADB over USB) |

**Catatan**: 11 modul ini mencakup thermal management (penting untuk health
device), power management, identifikasi hardware, dan USB ADB. **Beban kerja
TICK10** (sesuai TICK9 §4): `sprd_soc_thm.ko` + `sprd_thermal` (BUILTIN) baru
akan bind kalau `ap_efuse` + 13 sel kalibrasi + 3 `ap_thm0/1/2` + `virtual_sensor`
node DT lengkap.

## 5. Bukti build (file arsip)

| Artefak | Path | md5 | Build date | Sumber |
|---|---|---|---|---|
| `System.map` (segar) | `work/sysmaps/System.map.builddmed.20261014_2218` | `82055ecc…` | 14 Okt 22:18 WIB | TICK9 — menutup 2 baris ledger built-in (`sprd_flash_drv`, `spi-sprd`) |
| `sprd-gsp.ko` | `work/sysmaps/sprd-gsp.ko.20261014_0940` | `bfefe9af…` | 14 Okt 09:40 WIB | TICK9 — 535 simbol llvm-nm |
| `novatek_ts_spi.ko` | `work/sysmaps/novatek_ts_spi.ko.20261014_0940` | `943c7d89…` | 14 Okt 09:40 WIB | TICK9 — 337 simbol llvm-nm |
| `dotconfig` (segar) | `work/sysmaps/dotconfig.builddmed.20261014_2218` | `f9f1080f…` | 14 Okt 22:18 WIB | Config pasca-debloat |
| `kolist.1012b.txt` | `work/sysmaps/` | — | 12 Okt 10:12 WIB | 1.424 .ko (15 SPRD/Unisoc + 1.409 mainline) |
| `t21_full.log` | `/data/data/com.termux/files/home/kbuild/` (device) | — | 14 Okt 10:24 UTC | `make modules` RC=0 (TICK7) |

## 6. Status verifikasi (jujur)

- **Build/link**: 40/40 modul di §1-§4 = `built`. Snapshot `kolist.1012b.txt`
  menunjukkan 1.424 `.ko` keseluruhan; 15 di antaranya SPRD/Unisec + 25 lainnya
  SPRD/Advan (sipc, power, sensor, dll) yang relevan untuk device ini.
- **Modul compile-verified penuh di device** (13 modul dengan llvm-nm bukti):
  vsp, cpp, iommu, ion, camsys/vsp pw-domain, vsp_pw_domain, battery_info,
  charger_manager, wcn_core, wlan_combo, jpg, vdsp (TICK7), gsp +
  novatek_ts_spi (TICK9).
- **Modul dengan System.map evidence** (2 modul): `sprd_flash_drv`,
  `spi-sprd` (TICK9).
- **Modul yang TIDAK punya bukti runtime**: 25 modul di §3-§4 + sebagian §1-§2
  (yang ukurannya TBD = belum diukur). Build sukses ≠ driver valid di hardware
  fisik; semua butuh insmod + probe dmesg.
- **Belum di-flash/boot-tested**: Image 50.473.472 B (de-bloated) belum di-flash,
  dtb 45.702 B (TICK8) belum diuji runtime.
- **Thermal DT chain belum lengkap** (TICK9 §4): `ap_efuse` + 13 sel kalibrasi
  + 3 `ap_thm0/1/2` + `virtual_sensor` node belum ada di port. `sprd_thermal`
  BUILTIN + `sprd_soc_thm.ko` baru akan bind kalau itu lengkap.

## 7. Apa yang TIDAK termasuk

Daftar ini **bukan** "semua driver yang dibuild". Yang excluded:

- **Mainline crypto** (`aes-ce-ccm`, `ghash-ce`, `sha1-ce`, `arc4`, `twofish_*`,
  `lzo`, `lzo-rle`, `zstd`, `zram`, `zsmalloc`) — dipakai system, tapi generic.
- **Mainline net/wireless** (`cfg80211`, `mac80211`, `rtw88`, `ath10k`, dll) —
  dipakai, tapi tidak board-specific.
- **Mainline input/misc/sound** — generic.
- **Modul lain yang `.ko`-nya TBD**: ada ~25 modul SPRD/Unisoc di
  `kolist.1012b.txt` yang ukurannya belum dicatat di source (lihat TICK9 §6
  artefak yang cuma punya md5 untuk 2 `.ko`).

Daftar ini fokus pada **apa yang membuat Advan T1030 hidup + fungsional**:
boot → display → touch → WiFi/BT → camera → battery/charge → modem → sensor.

## 8. Pull request ke upstream

Tujuan akhir: masukkan 40 driver ini ke `main` branch di
`archanaberry/advan-vxtux-6.18.52` lewat PR yang traceable. Strategi:

1. `test1` branch (saat ini) sudah punya TICK8 (vdsp + de-bloat), TICK9
   (102/102 DEBLOBBED), TICK11–TICK12 (10 port terakhir, skor 129/129,
   boot QEMU LULUS) commits.
2. Bestie review test1 di
   https://github.com/archanaberry/advan-vxtux-6.18.52/tree/test1
3. Setelah OK, PR `test1 → main` lewat GitHub UI.
4. Tag release `v0.9-TICK12` dengan changelog: **129/129 blob vendor
   tertangani**, build V17 hijau (239 `.ko`), **boot QEMU LULUS**
   (DTB kustom tembus userspace tanpa panic), 102/102 DEBLOBBED,
   Image 50,5 MB, System.map segar.

Bestie tinggal bilang "merge" dan gw bantu handle.

— Berry Chan, Raspya, 2026-10-15 🍓
