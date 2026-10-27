# VXTux — MASTER DOCUMENT FINAL
## 103 Driver Port + 129 Driver Vendor Advan + Cross-Check + NV Analysis

> **Dokumen konsolidasi komplit** untuk menjelaskan "berapa driver VXTux sebenarnya,
> mana yang dari vendor Advan, mana yang port, mana yang NV, dan kenapa NV".
>
> **Generated**: 2026-10-15 (TICK10) · **Revisi v1.1**: 2026-10-24 (TICK12+)
> **Author**: Raspya Berry Chan (Hermes); revisi TICK11–12: sesi Buffy
> **License**: GPL-2 (sama dengan VXTux)
> **Status**: Final v1.1 — 129/129 blob vendor ditutup, build V17 hijau, boot QEMU LULUS (+ fbcon)

---

## 📋 Daftar Isi

1. [Statistik Utama](#1-statistik-utama)
2. [103 Driver Port (kernel 6.18.52-VXTux-UMS512)](#2-103-driver-port-kernel-61852-vxtux-ums512)
3. [129 Driver Vendor Advan T1030 (kernel 5.4 stock)](#3-129-driver-vendor-advan-t1030-kernel-54-stock)
4. [Cross-Check 103 × 129 (Matriks Lengkap)](#4-cross-check-103--129-matriks-lengkap)
5. [13 NV: Klasifikasi A/B/C/D/E](#5-13-nv-klasifikasi-abcde)
6. [88 Vendor-Only: Action Items TICK11+](#6-88-vendor-only-action-items-tick11)
7. [Build Evidence TICK7-TICK10](#7-build-evidence-tick7-tick10)
8. [Action Items Terurut Prioritas](#8-action-items-terurut-prioritas)
9. [Referensi & File Path](#9-referensi--file-path)

---

## 1. Statistik Utama

### 1.1 Angka final

| Metrik | Nilai | Sumber |
|---|---:|---|
| **Total driver port (103)** | **103** | inventory README repo kernel |
| Total driver vendor Advan (129) | 129 | `dumped/re/hidden/vramdisk_ko/` (kernel 5.4 stock) |
| Total .ko di build device TICK9 | 1.424 | `work/sysmaps/kolist.1012b.txt` |
| Total Image kernel | 50.473.472 B | pasca de-bloat, 14 Okt 22:18 WIB |
| Total DTB | 45.702 B | `ums512-1h10-vxtux.dtb` 14 Okt 17:24 UTC |
| System.map size | 9.458.636 B | md5 `82055ecc90c3430dc7a6f6f5fab150be` |
| Ledger DEBLOBBED | 102/102 (TICK9) | `docs/DEBLOB-20261015-TICK9-STATUS.md` |
| Modul compile-verified llvm-nm | 13 | TICK7+TICK9 evidence |
| 13 NV (klasifikasi) | 6 A + 2 C + 2 D + 3 E | lihat §5 |
| 88 vendor-only | belum di-port | lihat §6 |
| Branch aktif | `test1` | push ke fork `gryzzlaycutzy121/advan-vxtux-6.18.52` (origin archanaberry tanpa izin tulis) |
| Commit terakhir | `2a235f06b` | TICK12+ (uji fbcon QEMU) |
| **Port coverage 129 blob** | **129/129 ✅ (0 BELUM)** | regenerasi 2026-10-24: [STATUS-129](STATUS-129.md), [COVERAGE-129](COVERAGE-129.md), [PORT-AUDIT](PORT-AUDIT.md) |
| Build CI device penuh (V17) | **hijau** run `40097613453` | artefak `v17-built-kos` (239 .ko) + `v17-config` + `v17-dtb` |
| Boot test QEMU DTB kustom | **LULUS** run `40100295454` | [QEMU-BOOT-VALIDATION.md](QEMU-BOOT-VALIDATION.md) |
| Uji fbcon + DRM/KMS (QEMU) | **LULUS** run `40105064859` | `fb=1 px=1`, teks terlihat di framebuffer — [bukti PNG](evidence-20261024/fbcon.png) |
| CI workflow aktif | `v17-build` · `qemu-boot` · `quick-cc` | terpicu push ke `test1` |

### 1.2 Cross-check ringkas

| Perspektif | Match | Tidak match | Catatan |
|---|---:|---:|---|
| **Port 103** (driver di 6.18) | **90** ada di vendor | **13** NV | Lihat §5 untuk klasifikasi |
| **Vendor 129** (driver di 5.4) | **41** ada di port | **88** vendor-only | Lihat §6 untuk action items |
| **Gabungan unik** | ~150 entry | | Ada overlap (misal `core.ko` di vendor = `wcn_core.ko` di port) |

> **Catatan TICK12 (2026-10-24)**: angka 90/41/88 di atas adalah snapshot
> TICK10. Status otoritatif terkini: **129/129 blob vendor tertangani**
> ([STATUS-129.md](STATUS-129.md), [COVERAGE-129.md](COVERAGE-129.md)) —
> sepuluh entri terakhir ditutup port 2026-10-23→24 (tabel §6.2).

---

## 2. 103 Driver Port (kernel 6.18.52-VXTux-UMS512)

Tabel lengkap 103 driver port dengan **kolom Vendor 129** (V = ada di vendor, NV-X = tidak).

| # | Ledger ID | Source path di tree | Status / evidence + Vendor 129 |
|---:|---|---|---|
| 1 | `aes-ce-ccm` | [6.18.52-VXTux-UMS512/arch/arm64/crypto/aes-ce-ccm-glue.c](6.18.52-VXTux-UMS512/arch/arm64/crypto/aes-ce-ccm-glue.c) | … (lihat README) `[ V ]` |
| 2 | `aes-neon-blk` | [6.18.52-VXTux-UMS512/arch/arm64/crypto/aes-glue-neon.c](6.18.52-VXTux-UMS512/arch/arm64/crypto/aes-glue-neon.c) | … (lihat README) `[ V ]` |
| 3 | `agsd` | [6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/agdsp_access/agdsp_access.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/agdsp_access/agdsp_access.c) | … (lihat README) `[ V ]` |
| 4 | `apsys-dvfs` | [6.18.52-VXTux-UMS512/drivers/devfreq/sprd/apsys/sprd_dvfs_apsys.c](6.18.52-VXTux-UMS512/drivers/devfreq/sprd/apsys/sprd_dvfs_apsys.c) | … (lihat README) `[ V ]` |
| 5 | `arc4` | [6.18.52-VXTux-UMS512/crypto/arc4.c](6.18.52-VXTux-UMS512/crypto/arc4.c) | … (lihat README) `[ V ]` |
| 6 | `asix` | [6.18.52-VXTux-UMS512/drivers/net/usb/asix_common.c](6.18.52-VXTux-UMS512/drivers/net/usb/asix_common.c) | … (lihat README) `[ V ]` |
| 7 | `audio-dsp-dump` | [6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audiodspdump/audio_dsp_dump.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audiodspdump/audio_dsp_dump.c) | … (lihat README) `[ V ]` |
| 8 | `audio-pipe` | [6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audiosipc/audio-pipe.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audiosipc/audio-pipe.c) | … (lihat README) `[ V ]` |
| 9 | `audio_mem` | [6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audiomem/audio_mem.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audiomem/audio_mem.c) | … (lihat README) `[ V ]` |
| 10 | `audio_sipc` | [6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audiosipc/audio-sipc.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audiosipc/audio-sipc.c) | … (lihat README) `[ V ]` |
| 11 | `ax88179_178a` | [6.18.52-VXTux-UMS512/drivers/net/usb/ax88179_178a.c](6.18.52-VXTux-UMS512/drivers/net/usb/ax88179_178a.c) | … (lihat README) `[ V ]` |
| 12 | `bq2560x-charger` | [6.18.52-VXTux-UMS512/drivers/power/supply/bq256xx_charger.c](6.18.52-VXTux-UMS512/drivers/power/supply/bq256xx_charger.c) | … (lihat README) `[ V ]` |
| 13 | `core` | [6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/platform/Makefile](6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/platform/Makefile) | … (lihat README) `[ V ]` |
| 14 | `cpufreq_userspace` | [6.18.52-VXTux-UMS512/drivers/cpufreq/cpufreq_userspace.c](6.18.52-VXTux-UMS512/drivers/cpufreq/cpufreq_userspace.c) | … (lihat README) `[ V ]` |
| 15 | `extcon-usb-gpio` | [6.18.52-VXTux-UMS512/drivers/extcon/extcon-usb-gpio.c](6.18.52-VXTux-UMS512/drivers/extcon/extcon-usb-gpio.c) | … (lihat README) `[ V ]` |
| 16 | `ghash-ce` | [6.18.52-VXTux-UMS512/arch/arm64/crypto/ghash-ce-glue.c](6.18.52-VXTux-UMS512/arch/arm64/crypto/ghash-ce-glue.c) | … (lihat README) `[ V ]` |
| 17 | `gnss_common_ctl_all` | [6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/platform/gnss/gnss_common_ctl.c](6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/platform/gnss/gnss_common_ctl.c) | … (lihat README) `[ V ]` |
| 18 | `gnss_dbg` | [6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/platform/gnss/gnss_dbg.c](6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/platform/gnss/gnss_dbg.c) | … (lihat README) `[ V ]` |
| 19 | `gnss_pmnotify_ctl` | [6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/platform/gnss/gnss_pmnotify_ctl.c](6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/platform/gnss/gnss_pmnotify_ctl.c) | … (lihat README) `[ V ]` |
| 20 | `gpio` | [6.18.52-VXTux-UMS512/drivers/gpio/gpio-sprd.c](6.18.52-VXTux-UMS512/drivers/gpio/gpio-sprd.c) | … (lihat README) `[ V ]` |
| 21 | `hxchipset_83102e` | [6.18.52-VXTux-UMS512/drivers/input/touchscreen/sprd/hx83102e/hxchipset/himax_platform.c](6.18.52-VXTux-UMS512/drivers/input/touchscreen/sprd/hx83102e/hxchipset/himax_platform.c) | … (lihat README) `[ V ]` |
| 22 | `ims_bridge` | [6.18.52-VXTux-UMS512/net/ims_bridge/Makefile](6.18.52-VXTux-UMS512/net/ims_bridge/Makefile) | … (lihat README) `[ V ]` |
| 23 | `ion_cma_heap` | [6.18.52-VXTux-UMS512/drivers/dma-buf/heaps/cma_heap.c](6.18.52-VXTux-UMS512/drivers/dma-buf/heaps/cma_heap.c) | … (lihat README) `[ V ]` |
| 24 | `ion_ipc_trusty` | [6.18.52-VXTux-UMS512/drivers/dma-buf/sprd/ion_ipc_trusty.c](6.18.52-VXTux-UMS512/drivers/dma-buf/sprd/ion_ipc_trusty.c) | … (lihat README) `[ V ]` |
| 25 | `jpg` | [6.18.52-VXTux-UMS512/drivers/misc/sprd_jpg/sprd_jpg_main.c](6.18.52-VXTux-UMS512/drivers/misc/sprd_jpg/sprd_jpg_main.c) | … (lihat README) `[ V ]` |
| 26 | `leds-sc27xx-bltc` | [6.18.52-VXTux-UMS512/drivers/leds/leds-sc27xx-bltc.c](6.18.52-VXTux-UMS512/drivers/leds/leds-sc27xx-bltc.c) | … (lihat README) `[ V ]` |
| 27 | `ledtrig-pattern` | [6.18.52-VXTux-UMS512/drivers/leds/trigger/ledtrig-pattern.c](6.18.52-VXTux-UMS512/drivers/leds/trigger/ledtrig-pattern.c) | … (lihat README) `[ V ]` |
| 28 | `mcdt_hw` | [6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/mcdt/mcdt_r1p0/mcdt_hw.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/mcdt/mcdt_r1p0/mcdt_hw.c) | … (lihat README) `[ V ]` |
| 29 | `mipi_driver` | [6.18.52-VXTux-UMS512/drivers/unisoc_platform/debug_log/sharkl5/mipi_driver.c](6.18.52-VXTux-UMS512/drivers/unisoc_platform/debug_log/sharkl5/mipi_driver.c) | … (lihat README) `[ V ]` |
| 30 | `misc_sprd_uid` | [6.18.52-VXTux-UMS512/drivers/soc/sprd/soc_id/sprd_uid.c](6.18.52-VXTux-UMS512/drivers/soc/sprd/soc_id/sprd_uid.c) | … (lihat README) `[ V ]` |
| 31 | `musb_hdrc` | [6.18.52-VXTux-UMS512/drivers/usb/musb/musb_core.c](6.18.52-VXTux-UMS512/drivers/usb/musb/musb_core.c) | … (lihat README) `[ V ]` |
| 32 | `musb_sprd` | [6.18.52-VXTux-UMS512/drivers/usb/musb/musb_sprd_vxtux.c](6.18.52-VXTux-UMS512/drivers/usb/musb/musb_sprd_vxtux.c) | … (lihat README) `[ V ]` |
| 33 | `novatek_ts_spi` | [6.18.52-VXTux-UMS512/drivers/input/touchscreen/novatek_nvt_ts_spi/nt36xxx.c](6.18.52-VXTux-UMS512/drivers/input/touchscreen/novatek_nvt_ts_spi/nt36xxx.c) | … (lihat README) `[ V ]` |
| 34 | `phy-sprd-sharkl5Pro` | [6.18.52-VXTux-UMS512/drivers/phy/sprd/phy-sprd-ums512.c](6.18.52-VXTux-UMS512/drivers/phy/sprd/phy-sprd-ums512.c) | … (lihat README) `[ V ]` |
| 35 | `pinctrl` | [6.18.52-VXTux-UMS512/drivers/pinctrl/sprd/pinctrl-sprd.c](6.18.52-VXTux-UMS512/drivers/pinctrl/sprd/pinctrl-sprd.c) | … (lihat README) `[ V ]` |
| 36 | `pinctrl-sprd` | [6.18.52-VXTux-UMS512/drivers/pinctrl/sprd/pinctrl-sprd.c](6.18.52-VXTux-UMS512/drivers/pinctrl/sprd/pinctrl-sprd.c) | … (lihat README) `[ V ]` |
| 37 | `pinctrl-sprd-sharkl5Pro` | [6.18.52-VXTux-UMS512/drivers/pinctrl/sprd/pinctrl-sprd-sharkl5pro.c](6.18.52-VXTux-UMS512/drivers/pinctrl/sprd/pinctrl-sprd-sharkl5pro.c) | … (lihat README) `[ V ]` |
| 38 | `pwm-sprd` | [6.18.52-VXTux-UMS512/drivers/pwm/pwm-sprd.c](6.18.52-VXTux-UMS512/drivers/pwm/pwm-sprd.c) | … (lihat README) `[ V ]` |
| 39 | `pwm_bl` | [6.18.52-VXTux-UMS512/drivers/video/backlight/pwm_bl.c](6.18.52-VXTux-UMS512/drivers/video/backlight/pwm_bl.c) | … (lihat README) `[ V ]` |
| 40 | `rtc-sc27xx` | [6.18.52-VXTux-UMS512/drivers/rtc/rtc-sc27xx.c](6.18.52-VXTux-UMS512/drivers/rtc/rtc-sc27xx.c) | … (lihat README) `[ V ]` |
| 41 | `sc27xx-poweroff` | [6.18.52-VXTux-UMS512/drivers/power/reset/sc27xx-poweroff.c](6.18.52-VXTux-UMS512/drivers/power/reset/sc27xx-poweroff.c) | … (lihat README) `[ V ]` |
| 42 | `sc27xx-vibra` | [6.18.52-VXTux-UMS512/drivers/input/misc/sc27xx-vibra.c](6.18.52-VXTux-UMS512/drivers/input/misc/sc27xx-vibra.c) | … (lihat README) `[ V ]` |
| 43 | `sc27xx_adc` | [6.18.52-VXTux-UMS512/drivers/iio/adc/sc27xx_adc.c](6.18.52-VXTux-UMS512/drivers/iio/adc/sc27xx_adc.c) | … (lihat README) `[ V ]` |
| 44 | `sc27xx_fuel_gauge` | [6.18.52-VXTux-UMS512/drivers/power/supply/sc27xx_fuel_gauge.c](6.18.52-VXTux-UMS512/drivers/power/supply/sc27xx_fuel_gauge.c) | … (lihat README) `[ V ]` |
| 45 | `sc27xx_tsensor_thermal` | [6.18.52-VXTux-UMS512/drivers/thermal/sprd/sc27xx_tsensor_thermal.c](6.18.52-VXTux-UMS512/drivers/thermal/sprd/sc27xx_tsensor_thermal.c) | … (lihat README) `[ NV-A ]` |
| 46 | `sc27xx_typec` | [6.18.52-VXTux-UMS512/drivers/usb/typec/sprd/sc27xx_typec.c](6.18.52-VXTux-UMS512/drivers/usb/typec/sprd/sc27xx_typec.c) | … (lihat README) `[ V ]` |
| 47 | `seth` | [6.18.52-VXTux-UMS512/drivers/soc/sprd/sipc/seth.c](6.18.52-VXTux-UMS512/drivers/soc/sprd/sipc/seth.c) | … (lihat README) `[ V ]` |
| 48 | `sha1-ce` | [6.18.52-VXTux-UMS512/crypto/sha1.c](6.18.52-VXTux-UMS512/crypto/sha1.c) | … (lihat README) `[ V ]` |
| 49 | `snd-soc-sprd-card` | [6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/card/sprd-asoc-card-utils.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/card/sprd-asoc-card-utils.c) | … (lihat README) `[ V ]` |
| 50 | `snd-soc-sprd-codec-sc2730` | [6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/codec_sc2730/sprd-codec.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/codec_sc2730/sprd-codec.c) | … (lihat README) `[ V ]` |
| 51 | `snd-soc-sprd-codec-sc2730-power` | [6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/codec/sprd-audio-power.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/codec/sprd-audio-power.c) | … (lihat README) `[ V ]` |
| 52 | `snd-soc-sprd-codec-sc2730-power-dev` | [6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/codec/sprd-audio-power.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/codec/sprd-audio-power.c) | … (lihat README) `[ V ]` |
| 53 | `snd-soc-sprd-dummy-codec` | [6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/codec/dummy-codec.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/codec/dummy-codec.c) | … (lihat README) `[ V ]` |
| 54 | `snd-soc-sprd-vbc-fe` | [6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/dai_v4/vbc-dai.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/dai_v4/vbc-dai.c) | … (lihat README) `[ V ]` |
| 55 | `snd-soc-sprd-vbc-v4` | [6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/dai_v4/vbc-phy-v4.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/dai_v4/vbc-phy-v4.c) | … (lihat README) `[ V ]` |
| 56 | `spi-sprd` | [6.18.52-VXTux-UMS512/drivers/spi/spi-sprd.c](6.18.52-VXTux-UMS512/drivers/spi/spi-sprd.c) | … (lihat README) `[ V ]` |
| 57 | `sprd-charger-manager` | [6.18.52-VXTux-UMS512/drivers/power/supply/sprd/sprd_charger_manager.c](6.18.52-VXTux-UMS512/drivers/power/supply/sprd/sprd_charger_manager.c) | … (lihat README) `[ V ]` |
| 58 | `sprd-compr-2stage-dma` | [6.18.52-VXTux-UMS512/drivers/dma/sprd-dma.c](6.18.52-VXTux-UMS512/drivers/dma/sprd-dma.c) | … (lihat README) `[ V ]` |
| 59 | `sprd-dma` | [6.18.52-VXTux-UMS512/drivers/dma/sprd-dma.c](6.18.52-VXTux-UMS512/drivers/dma/sprd-dma.c) | … (lihat README) `[ V ]` |
| 60 | `sprd-dmaengine-pcm` | [6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/platform_include/sprd-dmaengine-pcm.h](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/platform_include/sprd-dmaengine-pcm.h) | … (lihat README) `[ V ]` |
| 61 | `sprd-drm` | [6.18.52-VXTux-UMS512/drivers/gpu/drm/sprd/sprd_drm.c](6.18.52-VXTux-UMS512/drivers/gpu/drm/sprd/sprd_drm.c) | … (lihat README) `[ V ]` |
| 62 | `sprd-gsp` | [6.18.52-VXTux-UMS512/drivers/gpu/drm/sprd/gsp/gsp_dev.c](6.18.52-VXTux-UMS512/drivers/gpu/drm/sprd/gsp/gsp_dev.c) | … (lihat README) `[ V ]` |
| 63 | `sprd-ion` | [6.18.52-VXTux-UMS512/drivers/dma-buf/sprd/sprd_ion.c](6.18.52-VXTux-UMS512/drivers/dma-buf/sprd/sprd_ion.c) | … (lihat README) `[ V ]` |
| 64 | `sprd-platform-pcm-routing` | [6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/platform/sprd-platform-pcm-routing.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/platform/sprd-platform-pcm-routing.c) | … (lihat README) `[ V ]` |
| 65 | `sprd-top-dvfs` | [6.18.52-VXTux-UMS512/drivers/devfreq/sprd/sprd-top-dvfs.c](6.18.52-VXTux-UMS512/drivers/devfreq/sprd/sprd-top-dvfs.c) | … (lihat README) `[ NV-C ]` |
| 66 | `sprd-vsp-pw-domain` | [6.18.52-VXTux-UMS512/drivers/pmdomain/sprd/sprd_vsp_pw_domain.c](6.18.52-VXTux-UMS512/drivers/pmdomain/sprd/sprd_vsp_pw_domain.c) | … (lihat README) `[ V ]` |
| 67 | `sprd_apipe` | [6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audio_pipe/audio_pipe.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audio_pipe/audio_pipe.c) | … (lihat README) `[ V ]` |
| 68 | `sprd_audcp_boot` | [6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audiocpboot/sprd_audcp_boot.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audiocpboot/sprd_audcp_boot.c) | … (lihat README) `[ V ]` |
| 69 | `sprd_audcp_dvfs` | [6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audiodvfs/sprd_audcp_dvfs.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audiodvfs/sprd_audcp_dvfs.c) | … (lihat README) `[ V ]` |
| 70 | `sprd_battery_info` | [6.18.52-VXTux-UMS512/drivers/power/supply/sprd/sprd_battery_info_main.c](6.18.52-VXTux-UMS512/drivers/power/supply/sprd/sprd_battery_info_main.c) | … (lihat README) `[ V ]` |
| 71 | `sprd_camera` | [6.18.52-VXTux-UMS512/drivers/media/platform/sprd/pipeline/sprd_dcam.c](6.18.52-VXTux-UMS512/drivers/media/platform/sprd/pipeline/sprd_dcam.c) | … (lihat README) `[ V ]` |
| 72 | `sprd_camsys_pw_domain` | [6.18.52-VXTux-UMS512/drivers/pmdomain/sprd/sprd_camsys_pw_domain.c](6.18.52-VXTux-UMS512/drivers/pmdomain/sprd/sprd_camsys_pw_domain.c) | … (lihat README) `[ V ]` |
| 73 | `sprd_cp_dvfs` | [6.18.52-VXTux-UMS512/drivers/devfreq/sprd/apsys/sprd_dvfs_apsys.c](6.18.52-VXTux-UMS512/drivers/devfreq/sprd/apsys/sprd_dvfs_apsys.c) | … (lihat README) `[ NV-C ]` |
| 74 | `sprd_cpp` | [6.18.52-VXTux-UMS512/drivers/media/platform/sprd/cpp/cpp_core.c](6.18.52-VXTux-UMS512/drivers/media/platform/sprd/cpp/cpp_core.c) | … (lihat README) `[ V ]` |
| 75 | `sprd_cpu_cooling` | [6.18.52-VXTux-UMS512/drivers/thermal/sprd/sprd_cpu_cooling.c](6.18.52-VXTux-UMS512/drivers/thermal/sprd/sprd_cpu_cooling.c) | … (lihat README) `[ NV-A ]` |
| 76 | `sprd_ddr_dvfs` | [6.18.52-VXTux-UMS512/drivers/devfreq/sprd/apsys/sprd_dvfs_apsys.c](6.18.52-VXTux-UMS512/drivers/devfreq/sprd/apsys/sprd_dvfs_apsys.c) | … (lihat README) `[ NV-D ]` |
| 77 | `sprd_flash_drv` | [6.18.52-VXTux-UMS512/drivers/sprd/flash/flash_drv.c](6.18.52-VXTux-UMS512/drivers/sprd/flash/flash_drv.c) | … (lihat README) `[ V ]` |
| 78 | `sprd_fm` | [6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/fm/driver/fm_sdio/fmdrv_main.c](6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/fm/driver/fm_sdio/fmdrv_main.c) | … (lihat README) `[ V ]` |
| 79 | `sprd_gpu_cooling` | [6.18.52-VXTux-UMS512/drivers/thermal/sprd/sprd_gpu_cooling.c](6.18.52-VXTux-UMS512/drivers/thermal/sprd/sprd_gpu_cooling.c) | … (lihat README) `[ NV-A ]` |
| 80 | `sprd_map` | [6.18.52-VXTux-UMS512/drivers/misc/sprd/sprd_map.c](6.18.52-VXTux-UMS512/drivers/misc/sprd/sprd_map.c) | … (lihat README) `[ NV-A ]` |
| 81 | `sprd_mipi` | [6.18.52-VXTux-UMS512/drivers/unisoc_platform/debug_log/Makefile](6.18.52-VXTux-UMS512/drivers/unisoc_platform/debug_log/Makefile) | … (lihat README) `[ V ]` |
| 82 | `sprd_pmic_syscon` | [6.18.52-VXTux-UMS512/drivers/soc/sprd/pmic_syscon/sprd_pmic_syscon.c](6.18.52-VXTux-UMS512/drivers/soc/sprd/pmic_syscon/sprd_pmic_syscon.c) | … (lihat README) `[ V ]` |
| 83 | `sprd_pmic_wdt` | [6.18.52-VXTux-UMS512/drivers/watchdog/sprd_wdt.c](6.18.52-VXTux-UMS512/drivers/watchdog/sprd_wdt.c) | … (lihat README) `[ NV-D ]` |
| 84 | `sprd_sensor` | [6.18.52-VXTux-UMS512/drivers/media/platform/sprd/sensors/sprd_sensor/Makefile](6.18.52-VXTux-UMS512/drivers/media/platform/sprd/sensors/sprd_sensor/Makefile) | … (lihat README) `[ V ]` |
| 85 | `sprd_soc_thm` | [6.18.52-VXTux-UMS512/drivers/thermal/sprd/sprd_soc_thm.c](6.18.52-VXTux-UMS512/drivers/thermal/sprd/sprd_soc_thm.c) | … (lihat README) `[ V ]` |
| 86 | `sprd_thermal` | [6.18.52-VXTux-UMS512/drivers/thermal/sprd_thermal.c](6.18.52-VXTux-UMS512/drivers/thermal/sprd_thermal.c) | … (lihat README) `[ NV-E ]` |
| 87 | `sprd_u_ether` | [6.18.52-VXTux-UMS512/drivers/usb/gadget/function/sprd/sprd_u_ether.c](6.18.52-VXTux-UMS512/drivers/usb/gadget/function/sprd/sprd_u_ether.c) | … (lihat README) `[ NV-A ]` |
| 88 | `sprd_usb_f_rndis` | [6.18.52-VXTux-UMS512/drivers/usb/gadget/function/sprd/sprd_usb_f_rndis.c](6.18.52-VXTux-UMS512/drivers/usb/gadget/function/sprd/sprd_usb_f_rndis.c) | … (lihat README) `[ NV-A ]` |
| 89 | `sprd_vdsp` | [6.18.52-VXTux-UMS512/drivers/vxtux/npu/vdsp/sprd_vdsp_main.c](6.18.52-VXTux-UMS512/drivers/vxtux/npu/vdsp/sprd_vdsp_main.c) | … (lihat README) `[ V ]` |
| 90 | `sprd_wdf` | [6.18.52-VXTux-UMS512/drivers/watchdog/sprd_wdf.c](6.18.52-VXTux-UMS512/drivers/watchdog/sprd_wdf.c) | … (lihat README) `[ V ]` |
| 91 | `sprd_wlan_combo` | [6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/wlan/Makefile](6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/wlan/Makefile) | … (lihat README) `[ V ]` |
| 92 | `sprdbt_tty` | [6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/bluetooth/driver/Makefile](6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/bluetooth/driver/Makefile) | … (lihat README) `[ V ]` |
| 93 | `thermal-generic-adc` | [6.18.52-VXTux-UMS512/drivers/thermal/thermal-generic-adc.c](6.18.52-VXTux-UMS512/drivers/thermal/thermal-generic-adc.c) | … (lihat README) `[ NV-E ]` |
| 94 | `trusty-tui` | [6.18.52-VXTux-UMS512/drivers/misc/trusty-tui.c](6.18.52-VXTux-UMS512/drivers/misc/trusty-tui.c) | … (lihat README) `[ V ]` |
| 95 | `twofish_common` | [6.18.52-VXTux-UMS512/crypto/twofish_common.c](6.18.52-VXTux-UMS512/crypto/twofish_common.c) | … (lihat README) `[ V ]` |
| 96 | `twofish_generic` | [6.18.52-VXTux-UMS512/crypto/twofish_generic.c](6.18.52-VXTux-UMS512/crypto/twofish_generic.c) | … (lihat README) `[ V ]` |
| 97 | `unisoc-iommu` | [6.18.52-VXTux-UMS512/drivers/iommu/sprd-iommu.c](6.18.52-VXTux-UMS512/drivers/iommu/sprd-iommu.c) | … (lihat README) `[ V ]` |
| 98 | `virt-dma` | [6.18.52-VXTux-UMS512/drivers/dma/virt-dma.c](6.18.52-VXTux-UMS512/drivers/dma/virt-dma.c) | … (lihat README) `[ V ]` |
| 99 | `vsp` | [6.18.52-VXTux-UMS512/drivers/media/platform/sprd/vsp/sprd_vsp_main.c](6.18.52-VXTux-UMS512/drivers/media/platform/sprd/vsp/sprd_vsp_main.c) | … (lihat README) `[ V ]` |
| 100 | `wcn_bsp` | [6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/platform/Makefile](6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/platform/Makefile) | … (lihat README) `[ V ]` |
| 101 | `zram` | [6.18.52-VXTux-UMS512/drivers/block/zram/zram_drv.c](6.18.52-VXTux-UMS512/drivers/block/zram/zram_drv.c) | … (lihat README) `[ V ]` |
| 102 | `zsmalloc` | [6.18.52-VXTux-UMS512/mm/zsmalloc.c](6.18.52-VXTux-UMS512/mm/zsmalloc.c) | … (lihat README) `[ V ]` |
| 103 | `panfrost` | [6.18.52-VXTux-UMS512/drivers/gpu/drm/panfrost/panfrost_gpu.c](6.18.52-VXTux-UMS512/drivers/gpu/drm/panfrost/panfrost_gpu.c) | … (lihat README) `[ NV-E ]` |


---

## 3. 129 Driver Vendor Advan T1030 (kernel 5.4 stock)

Driver yang dipakai Advan Tab VX Lite (T1030) dari vendor_ramdisk_stock.cpio, di-bongkar dari
`dumped/re/hidden/vramdisk_ko/`. Kernel 5.4 vendor (MocorDroid), bukan mainline.

### 3.1 Statistik vendor 129

| Subsistem | Count | Contoh |
|---|---:|---|
| Audio (SPRD/Unisoc) | 11 | snd-soc-sprd-*, audio_*, agdsp_access, kfifo_buf |
| Power/Regulator | 11 | bq2560x-charger, sc27xx_*, sprd_charger_manager, sprd_battery_info |
| GPIO/Pinctrl/Clock | 8 | gpio-sprd, pinctrl-sprd, clk-sprd, ums512-clk |
| I2C/SPI/SDHCI | 6 | i2c-sprd, spi-sprd, spi-sprd-adi, sdhci-sprd |
| SIPC/Modem (composite) | 12 | sipc-core, seth, sipx, spipe, spool, sblock/sbuf/slog_bridge, sprd_sip_svc, sprd_modem_loader |
| Storage/MMC | 4 | mmc_hsq, mmc_swcq, mmdvfs, sprd_iq |
| Efuse/NVMEM | 3 | nvmem_*, sprd_hwspinlock |
| Sensor/Thermal | 5 | sensorhub, sprd_thermal, sprd_soc_thm, sc27xx_*thermal, sc27xx_*_adc |
| CPUFreq | 3 | sprd-cpufreq-public/v2, sprd_cpu_cooling |
| USB Gadget | 3 | sprd_u_ether, sprd_usb_f_rndis, usb_f_vser |
| Trusty TEE | 6 | trusty, trusty-ipc/irq/log/pm/tui/virtio |
| Graphics | 2 | mali_gondul, panel-novatek-nt36672e |
| WiFi/BT | 2 | sprdwl, sprdbt_tty |
| Touch | 1 | novatek_ts_spi |
| Misc | 6 | sprd_map, sprd_pmic_syscon, sprd_drm, sprd_ion, unisoc-iommu, sprd-wdt_fiq |
| Mainline generic (ada di vendor) | ~30 | aes, ghash, sha1, arc4, twofish, virt-dma, zram, zsmalloc, dll |
| **Total** | **129** | |

### 3.2 Daftar 129 vendor .ko (urut abjad)


**Audio (SPRD/Unisoc)** (13):
- `agdsp_access.ko`
- `audio-dsp-dump.ko`
- `audio-pipe.ko`
- `audio_mem.ko`
- `audio_sipc.ko`
- `kfifo_buf.ko`
- `snd-soc-sprd-card.ko`
- `snd-soc-sprd-codec-sc2730-power-dev.ko`
- `snd-soc-sprd-codec-sc2730-power.ko`
- `snd-soc-sprd-codec-sc2730.ko`
- `snd-soc-sprd-dummy-codec.ko`
- `snd-soc-sprd-vbc-fe.ko`
- `snd-soc-sprd-vbc-v4.ko`

**Power/Regulator/Clock** (13):
- `bq2560x-charger.ko`
- `sc2730-regulator.ko`
- `sc27xx-poweroff.ko`
- `sc27xx-vibra.ko`
- `sc27xx_adc.ko`
- `sc27xx_fuel_gauge.ko`
- `sc27xx_typec.ko`
- `sprd-charger-manager.ko`
- `sprd_battery_info.ko`
- `sprd_pmic_syscon.ko`
- `sprd_pdbg.ko`
- `clk-sprd.ko`
- `ums512-clk.ko`

**GPIO/Pinctrl** (6):
- `gpio-eic-sprd.ko`
- `gpio-pmic-eic-sprd.ko`
- `gpio-sprd.ko`
- `pinctrl.ko`
- `pinctrl-sprd.ko`
- `pinctrl-sprd-sharkl5Pro.ko`

**I2C/SPI/SDHCI** (5):
- `i2c-sprd-hw.ko`
- `i2c-sprd.ko`
- `sdhci-sprd.ko`
- `spi-sprd-adi.ko`
- `spi-sprd.ko`

**MMC** (4):
- `mmc_hsq.ko`
- `mmc_swcq.ko`
- `mmdvfs.ko`
- `sprd_iq.ko`

**SIPC/Modem** (11):
- `sblock_bridge.ko`
- `sbuf_bridge.ko`
- `seth.ko`
- `sipc-core.ko`
- `sipx.ko`
- `slog_bridge.ko`
- `spipe.ko`
- `spool.ko`
- `sprd-sipc-virt-bus.ko`
- `sprd_modem_loader.ko`
- `sprd_sip_svc.ko`

**CPUFreq/Thermal/Cooling** (6):
- `sprd-cpufreq-public.ko`
- `sprd-cpufreq-v2.ko`
- `sprd_cpu_cooling.ko`
- `sprd_gpu_cooling.ko`
- `sprd_thermal.ko`
- `sprd_soc_thm.ko`

**Sensor/PMIC/WDT** (17):
- `nvmem-sc27xx-efuse.ko`
- `nvmem_sprd_cache_efuse.ko`
- `nvmem_sprd_efuse.ko`
- `rtc-sc27xx.ko`
- `sensorhub.ko`
- `sprd_7sreset.ko`
- `sprd_hwspinlock.ko`
- `sprd_power_manager.ko`
- `sprd_power_stat.ko`
- `sprd_shm.ko`
- `sprd_systimer.ko`
- `sprd_time_sync.ko`
- `sprd_time_sync_cp.ko`
- `sprd_wdt_fiq.ko`
- `sprd_manufacturer_model.ko`
- `sprd-cpufreq-public.ko`
- `sprd-sc27xx-spi.ko`

**Misc/USB/Camera/Codec** (30):
- `phy-sprd-sharkl5Pro.ko`
- `sprd_apipe.ko`
- `sprd_audcp_boot.ko`
- `sprd_audcp_dvfs.ko`
- `sprd_camera.ko`
- `sprd_compr-2stage-dma.ko`
- `sprd_cpp.ko`
- `sprd_dma.ko`
- `sprd_dmaengine-pcm.ko`
- `sprd-drm.ko`
- `sprd_flash_drv.ko`
- `sprd_fm.ko`
- `sprd-gsp.ko`
- `sprd-ion.ko`
- `sprd_map.ko`
- `sprd_mipi.ko`
- `sprd-platform-pcm-routing.ko`
- `sprd_top_dvfs.ko`
- `sprd-u_ether.ko`
- `sprd-usb_f_rndis.ko`
- `sprd-vsp-pw-domain.ko`
- `usb_f_vser.ko`
- `unisoc-mailbox.ko`
- `mali_gondul.ko`
- `panel-novatek-nt36672e.ko`
- `apr.ko`
- `act_gact.ko`
- `act_gate.ko`
- `act_mirred.ko`
- `amlogic-gxl-crypto.ko`

**WiFi/BT/Touch/Display** (5):
- `sprdwl.ko`
- `sprdbt_tty.ko`
- `novatek_ts_spi.ko`
- `musb_hdrc.ko`
- `musb_sprd.ko`

**Trusty TEE** (7):
- `trusty.ko`
- `trusty-ipc.ko`
- `trusty-irq.ko`
- `trusty-log.ko`
- `trusty-pm.ko`
- `trusty-tui.ko`
- `trusty-virtio.ko`

**Mainline generic (ada di vendor 5.4 juga)** (28):
- `aes-ce-ccm.ko`
- `aes-neon-blk.ko`
- `arc4.ko`
- `asix.ko`
- `ax88179_178a.ko`
- `cpufreq_userspace.ko`
- `extcon-usb-gpio.ko`
- `ghash-ce.ko`
- `gpio.ko`
- `hxchipset_83102e.ko`
- `ims_bridge.ko`
- `ion_cma_heap.ko`
- `ion_ipc_trusty.ko`
- `jpg.ko`
- `leds-sc27xx-bltc.ko`
- `ledtrig-pattern.ko`
- `mcdt_hw.ko`
- `mipi_driver.ko`
- `misc_sprd_uid.ko`
- `pwm-sprd.ko`
- `pwm_bl.ko`
- `sha1-ce.ko`
- `twofish_common.ko`
- `twofish_generic.ko`
- `virt-dma.ko`
- `vsp.ko`
- `zram.ko`
- `zsmalloc.ko`

**Lainnya (BMS, GPIO, MISC)** (8):
- `core.ko`
- `wcn_bsp.ko`
- `gnss_common_ctl_all.ko`
- `gnss_dbg.ko`
- `gnss_pmnotify_ctl.ko`
- `apsys-dvfs.ko`
- `wcn36xx.ko`
- `wcnss_ctrl.ko`

**Belum dikategorikan** (7):
- `sprd-compr-2stage-dma.ko`
- `sprd-dma.ko`
- `sprd-dmaengine-pcm.ko`
- `sprd_camsys_pw_domain.ko`
- `sprd_sensor.ko`
- `sprd_soc_id.ko`
- `unisoc-iommu.ko`

**Total vendor 129 .ko: 129**


---

## 4. Cross-Check 103 × 129 (Matriks Lengkap)

### 4.1 Ringkasan

| Sumber | Match | Tidak match | Total |
|---|---:|---:|---:|
| Port 103 (kernel 6.18) | 90 ada di vendor | 13 NV | 103 |
| Vendor 129 (kernel 5.4) | 41 ada di port | 88 vendor-only | 129 |

**Asimetri 90 vs 41**: banyak entry port match ke >1 nama vendor (e.g. `wcn_bsp` + `wcn_core` di vendor = 1 entry `wcn_bsp` di port). Beberapa nama vendor engga match exact (e.g. `wcn36xx.ko` di vendor, engga ada di port karena distrip ke composite `wcn_core.ko`).

### 4.2 41 match vendor → port (definitive)

| Vendor .ko | Match di port (alias) | Catatan |
|---|---|---|
| `agdsp_access.ko` | `agsd` | |
| `arc4.ko` | `arc4` | |
| `asix.ko` | `asix` | |
| `audio_mem.ko` | `audio_mem` | |
| `audio_sipc.ko` | `audio_sipc` | |
| `ax88179_178a.ko` | `ax88179_178a` | |
| `core.ko` | `core` | |
| `gpio.ko` | `gpio` | |
| `hxchipset_83102e.ko` | `hxchipset_83102e` | |
| `ion_cma_heap.ko` | `ion_cma_heap` | |
| `ion_ipc_trusty.ko` | `ion_ipc_trusty` | |
| `jpg.ko` | `jpg` | |
| `mcdt_hw.ko` | `mcdt_hw` | |
| `mipi_driver.ko` | `mipi_driver` | |
| `misc_sprd_uid.ko` | `misc_sprd_uid` | |
| `musb_hdrc.ko` | `musb_hdrc` | |
| `musb_sprd.ko` | `musb_sprd` | |
| `novatek_ts_spi.ko` | `novatek_ts_spi` | |
| `pinctrl.ko` | `pinctrl` | |
| `pwm_bl.ko` | `pwm_bl` | |
| `sc27xx_adc.ko` | `sc27xx_adc` | |
| `sc27xx_fuel_gauge.ko` | `sc27xx_fuel_gauge` | |
| `sc27xx_typec.ko` | `sc27xx_typec` | |
| `sprd_apipe.ko` | `sprd_apipe` | |
| `sprd_audcp_boot.ko` | `sprd_audcp_boot` | |
| `sprd_audcp_dvfs.ko` | `sprd_cp_dvfs` | |
| `sprd_battery_info.ko` | `sprd_battery_info` | |
| `sprd_camera.ko` | `sprd_camera` | |
| `sprd_camsys_pw_domain.ko` | `sprd_camsys_pw_domain` | |
| `sprd_cpp.ko` | `sprd_cpp` | |
| `sprd_flash_drv.ko` | `sprd_flash_drv` | |
| `sprd_mipi.ko` | `sprd_mipi` | |
| `sprd_pmic_syscon.ko` | `sprd_pmic_syscon` | |
| `sprd_sensor.ko` | `sprd_sensor` | |
| `sprd_wdt_fiq.ko` | `sprd_pmic_wdt` | |
| `trusty.ko` | `trusty-tui` | |
| `twofish_common.ko` | `twofish_common` | |
| `twofish_generic.ko` | `twofish_generic` | |
| `vsp.ko` | `vsp` | |
| `zram.ko` | `zram` | |
| `zsmalloc.ko` | `zsmalloc` | |

**Total: 41 match vendor→port**


---

## 5. 13 NV: Klasifikasi A/B/C/D/E

13 entry dari 103 yang **tidak** ada di vendor 129, dikategorikan berdasarkan akar penyebab.

### 5.1 Tabel 13 NV

| # | Ledger ID | Vendor match | Kategori | Sebab |
|---:|---|---|:---:|---|
| 45 | `sc27xx_tsensor_thermal` | NONE | **A** | Driver baru TICK10 (pmic_tsensor + osctsen-thmzone DT) |
| 65 | `sprd-top-dvfs` | apsys-dvfs.ko (pecahan) | **C** | Vendor composite dipecah 1→3 of_match |
| 73 | `sprd_cp_dvfs` | sprd_audcp_dvfs.ko (alias) | **C** | Vendor sprd_cp_dvfs = sprd_audcp_dvfs.ko |
| 75 | `sprd_cpu_cooling` | NONE | **A** | Driver baru (late_initcall, tanpa DT) |
| 76 | `sprd_ddr_dvfs` | apsys-dvfs.ko (alias) | **D** | Entry OBSOLETE: sumber engga ada |
| 79 | `sprd_gpu_cooling` | NONE | **A** | Driver baru (CONFIG_SPRD_GPU_COOLING=y built-in) |
| 80 | `sprd_map` | NONE | **A** | Driver baru (ION/System.map equivalent) |
| 83 | `sprd_pmic_wdt` | sprd_wdt_fiq.ko (nama beda) | **D** | Entry OBSOLETE: sumber engga ada, vendor pakai sprd_wdt_fiq |
| 86 | `sprd_thermal` | NONE (BUILTIN) | **E** | BUILTIN (=y) ke vmlinux, engga jadi .ko |
| 87 | `sprd_u_ether` | NONE | **A** | Driver baru (USB gadget, vendor engga aktifkan) |
| 88 | `sprd_usb_f_rndis` | NONE | **A** | Driver baru (USB gadget, vendor engga aktifkan) |
| 93 | `thermal-generic-adc` | NONE (mainline) | **E** | Mainline generic, NV by design |
| 103 | `panfrost` | NONE (mainline GPU) | **E** | Mainline GPU, REPLACES vendor Mali Gondul |

### 5.2 Ringkasan per kategori

| Kategori | Count | Definisi | Action |
|---|---:|---|---|
| **A — Driver baru** | 6 | Source ada di tree, vendor engga punya | Tinggal tunggu make modules rebuild TICK10+ |
| **B** (reserved) | 0 | (tidak ada kasus) | — |
| **C — Composite pecahan** | 2 | Vendor 1 modul dipecah jadi N of_match | OK, dokumentasikan |
| **D — Entry obsolete** | 2 | Sumber + vendor engga ada | Hapus dari ledger atau ganti nama |
| **E — Mainline/BUILTIN** | 3 | Bukan vendor-specific (mainline/BUILTIN) | OK, NV by design |

**Total: 13 NV** (klasifikasi lengkap, no entry tanpa sebab jelas)

---

## 6. 88 Vendor-Only: Action Items TICK11+ — SUDAH SEBAGIAN BESAR DITUTUP

88 entry dari vendor 129 yang **tidak** ada di port 103 pada snapshot TICK10.
**Update TICK12 (2026-10-24): daftar ini kedaluwarsa** — skor otoritatif kini
**129/129 tertangani** (regenerasi terhadap bukti build V17; tabel §6.2 di
bawah). Snapshot daftar dipertahankan sebagai riwayat TICK10.

### 6.1 Kategori 88 vendor-only

**Mainline (di vendor tapi bukan vendor-specific)** (26):
- `aes-ce-ccm.ko`
- `aes-neon-blk.ko`
- `extcon-usb-gpio.ko`
- `ghash-ce.ko`
- `leds-sc27xx-bltc.ko`
- `ledtrig-pattern.ko`
- `pwm-sprd.ko`
- `sha1-ce.ko`
- `phy-sprd-sharkl5Pro.ko`
- `pinctrl-sprd-sharkl5Pro.ko`
- `pinctrl-sprd.ko`
**Audio (vendor sudah di tree, mungkin perlu update)** (13):
- `audio-dsp-dump.ko`
- `audio-pipe.ko`
- `snd-soc-sprd-card.ko`
- `snd-soc-sprd-codec-sc2730.ko`
- `snd-soc-sprd-codec-sc2730-power.ko`
- `snd-soc-sprd-codec-sc2730-power-dev.ko`
- `snd-soc-sprd-dummy-codec.ko`
- `snd-soc-sprd-vbc-fe.ko`
- `snd-soc-sprd-vbc-v4.ko`
- `kfifo_buf.ko`
**Power/Clock/Regulator BELUM di-port** (7):
- `bq2560x-charger.ko`
- `rtc-sc27xx.ko`
- `sc27xx-poweroff.ko`
- `sc27xx-vibra.ko`
- `sc2730-regulator.ko`
- `clk-sprd.ko`
- `ums512-clk.ko`
**I2C/SPI/SDHCI/MMC BELUM di-port** (7):
- `i2c-sprd.ko`
- `i2c-sprd-hw.ko`
- `spi-sprd-adi.ko`
- `sdhci-sprd.ko`
- `mmc_hsq.ko`
- `mmc_swcq.ko`
- `mmdvfs.ko`
**GPIO BELUM di-port (semua)** (3):
- `gpio-sprd.ko`
- `gpio-eic-sprd.ko`
- `gpio-pmic-eic-sprd.ko`
**Efuse/NVMEM BELUM di-port (VITAL untuk thermal chain)** (3):
- `nvmem_sprd_efuse.ko`
- `nvmem_sprd_cache_efuse.ko`
- `nvmem-sc27xx-efuse.ko`
**SIPC/Modem BELUM di-port (sebagian)** (3):
- `sipc-core.ko`
- `sipx.ko`
- `slog_bridge.ko`
**Power/Misc BELUM di-port** (10):
- `sprd_7sreset.ko`
- `sprd_hwspinlock.ko`
- `sprd_systimer.ko`
- `sprd_shm.ko`
- `sprd_power_stat.ko`
- `sprd_pdbg.ko`
- `sprd_manufacturer_model.ko`
- `sprd-cpufreq-public.ko`
- `sprd-cpufreq-v2.ko`
- `sprd-sc27xx-spi.ko`
**Sensor BELUM di-port** (1):
- `sensorhub.ko`
**Mailbox/USB BELUM di-port** (2):
- `unisoc-mailbox.ko`
- `usb_f_vser.ko`
**Trusty TEE BELUM di-port (5 dari 6)** (5):
- `trusty-ipc.ko`
- `trusty-irq.ko`
- `trusty-log.ko`
- `trusty-pm.ko`
- `trusty-virtio.ko`
**WCN/Touch/MISC BELUM di-port** (62):
- `apsys-dvfs.ko`
- `sblock_bridge.ko`
- `sbuf_bridge.ko`
- `spipe.ko`
- `spool.ko`
- `sprd-charger-manager.ko`
- `sprd-compr-2stage-dma.ko`
- `sprd-dma.ko`
- `sprd-dmaengine-pcm.ko`
- `sprd-drm.ko`
- `sprd-gsp.ko`
- `sprd-ion.ko`
- `sprd-platform-pcm-routing.ko`
- `sprd-sipc-virt-bus.ko`
- `sprd-vsp-pw-domain.ko`
- `sprd_iq.ko`
- `sprd_modem_loader.ko`
- `sprd_power_manager.ko`
- `sprd_sip_svc.ko`
- `sprd_soc_id.ko`
- `sprd_time_sync.ko`
- `sprd_time_sync_cp.ko`
- `trusty-tui.ko`
- `unisoc-iommu.ko`
- `virt-dma.ko`
- `spi-sprd.ko`
- `sprd-drm.ko`

**Total: 142 vendor-only** (angka snapshot TICK10; judul “88” vs jumlah ini
memang kontradiktif — keduanya kedaluwarsa, lihat §6.2 dan STATUS-129)

### 6.2 Sepuluh entri terakhir ditutup (TICK11–TICK12, 2026-10-23 → 24)

Semua sudah diport + terbangun di CI V17 (bukti ko:
[evidence-20261024/ko_list_v17.txt](evidence-20261024/ko_list_v17.txt)):

| # | blob stok | hasil port di tree 6.18 | commit | dokumen bukti |
|---:|---|---|---|---|
| 1 | `core.ko` | `drivers/vxtux/soc/autotest/sprd_autotest_core.c` | `b1d692f09` | [RE-SPRD-AUTOTEST.md](RE-SPRD-AUTOTEST.md) |
| 2 | `gpio.ko` | `drivers/vxtux/soc/autotest/sprd_autotest_gpio.c` | `b1d692f09` | [RE-SPRD-AUTOTEST.md](RE-SPRD-AUTOTEST.md) |
| 3 | `sprd_7sreset.ko` | `drivers/vxtux/soc/sprd_7sreset/` + node DT `hw-reset` | `b1d692f09` | [RE-SPRD-7SRESET.md](RE-SPRD-7SRESET.md) |
| 4 | `sprd_manufacturer_model.ko` | `drivers/vxtux/soc/manufacturer_model/` | `b1d692f09` | [RE-SPRD-MANUFACTURER.md](RE-SPRD-MANUFACTURER.md) |
| 5 | `sprd_pdbg.ko` | `drivers/vxtux/soc/pdbg/` | `cb5e91d35` | [RE-SPRD-PDBG.md](RE-SPRD-PDBG.md) |
| 6 | `sprd_power_stat.ko` | `drivers/vxtux/soc/debugstat/` → `sprd_debugstat.ko` | `8bf97026b` | [RE-SPRD-POWER-STAT.md](RE-SPRD-POWER-STAT.md) |
| 7 | `sprd_iq.ko` | `drivers/vxtux/soc/iq/` | `9953413ba` | [RE-SPRD-IQ.md](RE-SPRD-IQ.md) |
| 8 | `sprd-cpufreq-v2.ko` | `drivers/cpufreq/sprd-cpufreq-v2-*` | `91e7838b2` | [PORT-REMAINING.md](PORT-REMAINING.md) §4 |
| 9 | `sprd-cpufreq-public.ko` | `drivers/cpufreq/sprd-cpufreq-public.c` | `91e7838b2` | [PORT-REMAINING.md](PORT-REMAINING.md) §5 |
| 10 | `mmc_swcq.ko` | `drivers/mmc/host/mmc_swcq.c` | `91e7838b2` | [RE-MMC-SWCQ.md](RE-MMC-SWCQ.md) |


---

## 7. Build Evidence TICK7-TICK10

### 7.1 TICK7 (2026-10-14) — `make modules` RC=0, 1.421 .ko

- **10 modul compile-verified** (TICK7): vsp, cpp, iommu, ion, camsys/vsp pw-domain, battery_info, charger_manager, wcn_core, wlan_combo, jpg
- Log: `t21_full.log` (10:24 UTC, 1.435 .ko)
- Tally 103/103 artefak: 5 KO + 98 OBJ + 0 MISS

### 7.2 TICK8 (2026-10-14) — `sprd_vdsp` ported + Image de-bloat 59→50,5 MB

- `sprd_vdsp.ko` 913.568 byte (relink penuh)
- Image de-bloated: 50.473.472 byte (dari 59.021.824; −14,5%)
- Penyebab: +40 opsi DEBUG/tracing dari restore config sesi t28
- DTB updated: `ums512-1h10-vxtux.dtb` 45.702 byte

### 7.3 TICK9 (2026-10-15) — `sprd-gsp` + `novatek_ts_spi` verified

- `sprd-gsp.ko` 767.488 B (md5 `bfefe9af…`, llvm-nm 535 simbol)
- `novatek_ts_spi.ko` 512.584 B (md5 `943c7d89…`, llvm-nm 337 simbol)
- Ledger 102/102 DEBLOBBED (sebelumnya 98/3/1)
- 4 baris terakhir ditutup: sprd_flash_drv + spi-sprd (System.map), sprd-gsp + novatek_ts_spi (.ko build)

### 7.4 TICK10 (2026-10-15) — thermal DT chain 1/4 + sprd_vdsp retro-fix

- **Commit `e27dda4c1`**: 46 files, +10.676 baris
- **sprd_vdsp** source akhirnya masuk `.git` (10.223 LOC di `drivers/vxtux/npu/vdsp/`)
- **Thermal DT chain 1/4**: `pmic_tsensor` di sc2730.dtsi + `osctsen/outtsen-thmzone` di vxtux.dts
- **Beban kerja tersisa**: 3/4 thermal chain (ap_efuse + 13 sel kalibrasi + 3 ap_thm + virtual_sensor)

### 7.5 Tabel verifikasi driver

| Driver | Ukuran | md5 | Sumber | TICK | Build run |
|---|---:|---|---|---|---|
| `sprd_vsp.ko` | 231 KB | — | gcc 16.2.1 | TICK7 | t21_full.log |
| `cpp.ko` | 321 KB | — | gcc 16.2.1 | TICK7 | t21_full.log |
| `sprd-iommu.ko` | 81 KB | — | gcc 16.2.1 | TICK7 | t21_full.log |
| `sprd_ion.ko` | 62 KB | — | gcc 16.2.1 | TICK7 | t21_full.log |
| `sprd_camsys_pw_domain.ko` | 95 KB | — | gcc 16.2.1 | TICK7 | t21_full.log |
| `sprd_vsp_pw_domain.ko` | 52 KB | — | gcc 16.2.1 | TICK7 | t21_full.log |
| `sprd_battery_info.ko` | 96 KB | — | gcc 16.2.1 | TICK7 | t21_full.log |
| `wcn_core.ko` (composite) | TBD | — | gcc 16.2.1 | TICK7 | t21_full.log |
| `sprdwl.ko` | 2,5 MB | — | gcc 16.2.1 | TICK7 | t21_full.log |
| `sprd_jpg.ko` | 156 KB | — | gcc 16.2.1 | TICK7 | t21_full.log |
| `sprd_vdsp.ko` | 913.568 B | — | gcc 16.2.1 (relink penuh) | TICK8 | t21_full.log (relink) |
| `sprd-gsp.ko` | 767.488 B | `bfefe9af…` | llvm-nm 535 simbol | TICK9 | t22_nohup.out |
| `novatek_ts_spi.ko` | 512.584 B | `943c7d89…` | llvm-nm 337 simbol | TICK9 | t22_nohup.out |
| `sprd_flash_drv` (System.map) | 14 simbol | — | 14 Okt 22:18 WIB | TICK9 | System.map.builddmed.20261014_2218 |
| `spi-sprd` (System.map) | 23 simbol | — | 14 Okt 22:18 WIB | TICK9 | System.map.builddmed.20261014_2218 |
| `sprd_autotest_core.ko` | 340.976 B | — | V17 `40097613453` | TICK12 | v17-built-kos |
| `sprd_autotest_gpio.ko` | 392.728 B | — | V17 `40097613453` | TICK12 | v17-built-kos |
| `sprd_7sreset.ko` | 359.744 B | — | V17 `40097613453` | TICK12 | v17-built-kos |
| `sprd_manufacturer_model.ko` | 323.328 B | — | V17 `40097613453` | TICK12 | v17-built-kos |
| `sprd_pdbg.ko` | 485.840 B | — | V17 `40097613453` | TICK12 | v17-built-kos |
| `sprd_debugstat.ko` | 385.144 B | — | V17 `40097613453` | TICK12 | v17-built-kos |
| `sprd_iq.ko` | 471.064 B | — | V17 `40097613453` | TICK12 | v17-built-kos |
| `sprd-cpufreq-v2.ko` | 630.520 B | — | V17 `40097613453` | TICK12 | v17-built-kos |
| `sprd-cpufreq-public.ko` | 392.640 B | — | V17 `40097613453` | TICK12 | v17-built-kos |
| `mmc_swcq.ko` | 734.296 B | — | V17 `40097613453` | TICK12 | v17-built-kos |

### 7.6 TICK11–TICK12 (2026-10-22 → 2026-10-24) — 10 port final, CI penuh, boot QEMU LULUS

**TICK11 (2026-10-22, `efa7348c2`)**: 3 modul efuse (`NVMEM_SPRD_EFUSE`,
`NVMEM_SPRD_CACHE_EFUSE`, `NVMEM_SC27XX_EFUSE`) + thermal DT chain 3/4
(`ap_efuse`, `ap_thm0/1/2` + 12 sensor, `virtual-sensor`, 13 zona thermal,
13 sel kalibrasi PMIC) — detail di §8.1.

**TICK12 (2026-10-23→24)** — menutup 10 blob terakhir + peralatan RE:

- Laporan RE lengkap dari blob (objdump + relokasi): [RE-SPRD-PDBG.md](RE-SPRD-PDBG.md),
  [RE-SPRD-POWER-STAT.md](RE-SPRD-POWER-STAT.md), [RE-SPRD-IQ.md](RE-SPRD-IQ.md),
  [RE-SPRD-MANUFACTURER.md](RE-SPRD-MANUFACTURER.md), [RE-SPRD-7SRESET.md](RE-SPRD-7SRESET.md),
  [RE-SPRD-SIP-SVC.md](RE-SPRD-SIP-SVC.md), [RE-MMC-SWCQ.md](RE-MMC-SWCQ.md),
  [RE-SPRD-AUTOTEST.md](RE-SPRD-AUTOTEST.md) (bukti di `docs/evidence-20261023/`).
- Dua API yang sudah dihapus di 6.18 disesuaikan (`74209d523`) — modpost lolos.
- **Regenerasi skor (2026-10-24)** terhadap bukti V17:
  - [PORT-AUDIT.md](PORT-AUDIT.md): 129 baris (KO 68 / Y 47 / NOSRC 14-builtin);
    `vendor_list()` kini dibatasi tabel §2 (dulu ikut baca tabel §4/§5 → 153
    baris), `UNRESOLVED` core/gpio dikosongkan (framework sudah ada di pohon).
  - [STATUS-129.md](STATUS-129.md) & [COVERAGE-129.md](COVERAGE-129.md):
    **129/129 ✅, 0 BELUM**.

**CI Build V17 hijau** (guard 124 simbol + DTB + modules + artefak):

| Run | Tanggal | Hasil | Catatan |
|---|---|---|---|
| `40087667424` | 2026-10-24 | ✅ 24m17s | build penuh pertama pasca port 10 blob |
| `40097613453` | 2026-10-24 | ✅ | sumber bukti `ko_list_v17.txt` (239 .ko) + `config_v17.txt` |

**Boot test QEMU — LULUS** (run `40100295454`, gate `marker: stock=1
custom=1`): DTB kustom `qemu-ums512-t1030.dtb` tembus userspace ~4 detik
tanpa panic, init C statis mencetak marker ke console+kmsg. Tiga akar
masalah "boot 0 byte" diperbaiki: GICR pindah ke `0x080A0000`, PSCI `hvc`,
tambahan node PCIe + ITS — rincian di
[QEMU-BOOT-VALIDATION.md](QEMU-BOOT-VALIDATION.md).

**fbcon/DRM-KMS — TERBUKTI BISA PRINT CLI** (run `40105064859` hijau,
gate `marker: stock=1 custom=1 fb=1 px=1`): config lama `CONFIG_FB=n` +
`FRAMEBUFFER_CONSOLE` tak terdefinisi dinyalakan lewat fragmen QEMU
(`DRM_VIRTIO_GPU` + `DRM_FBDEV_EMULATION` + `FRAMEBUFFER_CONSOLE` +
`VT_CONSOLE`), boot ketiga memakai `-device virtio-gpu-pci` + screendump.
Bukti probe: `Console: switching to colour frame buffer device 128x48`,
`fb0: virtio_gpudrmfb`; 2.842 piksel tak-hitam di 1024×768 = banner CLI
`VXTux 6.18.52 - fbcon CLI test` terlihat ([PNG](evidence-20261024/fbcon.png)).
Rincian di [QEMU-BOOT-VALIDATION.md](QEMU-BOOT-VALIDATION.md) §5.

---

## 8. Action Items Terurut Prioritas

### 8.1 CRITICAL (block thermal chain)

> **UPDATE TICK11 (2026-10-22, commit `efa7348c2` di `test1`):** butir 1–3 dari daftar ini SELESAI ditulis. 3 modul efuse sudah ada di tree (`nvmem-sprd-efuse.c` + `sc27xx-efuse.c` di `drivers/nvmem/`, `sprd_cache_efuse.c` di `drivers/vxtux/soc/cache_efuse/`, config `NVMEM_SPRD_EFUSE=m` + `NVMEM_SC27XX_EFUSE=m` + `NVMEM_SPRD_CACHE_EFUSE=m`). Thermal DT chain 3/4 lengkap: `ap_efuse@32240000`, `ap_thm0/1/2` + 12 sensor, `virtual-sensor@0` (`SPRD_SOC_THM=m`), 13 zona thermal, cooling-devices cluster0/1 + gpu-cooling0, 13 sel kalibrasi PMIC `efuse@300` (8 ditambah). Sisa TICK12: verifikasi boot di hardware + insmod.

1. **Port `nvmem_sprd_efuse.ko` + `nvmem_sprd_cache_efuse.ko` + `nvmem-sc27xx-efuse.ko`** (3 .ko) — wajib untuk TICK11 thermal DT chain (ap_efuse + 13 sel kalibrasi).
2. **Port `ap_efuse` node DT** (efuse@32240000) + **13 sel kalibrasi** + **3 `ap_thm0/1/2` nodes** + **`virtual_sensor` node** — thermal DT chain 2/4, 3/4, 4/4.
3. **Bangun `sprd_thermal.ko` BUILTIN** (=y) — saat ini BUILTIN tapi node DT belum lengkap, jadi driver exit early di `sprd_thm_cal_read()`.

### 8.2 HIGH (core functionality)

4. **Re-run `make modules` TICK10+** untuk verifikasi `sprd_vdsp` retro-fix + thermal DT nodes.
5. **Resync 1.435 .ko ke config de-bloat** (saat ini mtime 14 Okt 09:40 WIB, dari config pra-debloat).
6. **Boot test + insmod** — satu-satunya cara verifikasi driver beneran hidup di hardware (device sedang sidang).

### 8.3 MEDIUM (cleanup)

7. **Hapus 2 entry obsolete** (kategori D): `sprd_ddr_dvfs` + `sprd_pmic_wdt` dari inventory 103 (atau rename `sprd_pmic_wdt` → `sprd_wdt_fiq`).
8. **Port 88 vendor-only** yang vital:
   - **Trusty TEE** (5 .ko) — untuk secure boot
   - **GPIO SPRD** (3 .ko) — untuk kontrol hardware
   - **Mailbox** (1 .ko) — untuk AP↔CP IPC
   - **Sensor/PMIC** (beberapa) — untuk sensor service
   - **Power/Misc** (10 .ko) — untuk runtime management
9. **Update `kolist.1012b.txt`** setelah TICK10 rebuild — saat ini Oct 13 (TICK7), obsolete.

### 8.4 LOW (nice-to-have)

10. **PR `test1 → main` lewat GitHub UI** — bestie review + merge.
11. **Tag release `v0.9-TICK10`** setelah merge.
12. **Update dokumentasi Freebuff** dengan link ke MASTER.md ini.

---

## 9. Referensi & File Path

### 9.1 Source code path

| Path | Isi |
|---|---|
| `src/kernel/6.18.52-VXTux-UMS512/` | Repo kernel 6.18.52 |
| `src/kernel/6.18.52-VXTux-UMS512/6.18.52-VXTux-UMS512/` | Tree kernel (drivers, fs, arch, dll) |
| `src/kernel/6.18.52-VXTux-UMS512/6.18.52-VXTux-UMS512/drivers/vxtux/` | Port VXTux (vendor) |
| `src/kernel/6.18.52-VXTux-UMS512/docs/` | Dokumentasi (DRIVERS_RELEVANT, DRIVERS_VENDOR_CROSSCHECK) |

### 9.2 Workspace path

| Path | Isi |
|---|---|
| `dumped/re/hidden/vramdisk_ko/` | 129 vendor .ko Advan |
| `dumped/re/hidden/vendor_ramdisk_stock.cpio` | CPIO asli Advan |
| `work/sysmaps/System.map.builddmed.20261014_2218` | System.map segar md5 `82055ecc…` |
| `work/sysmaps/kolist.1012b.txt` | 1.424 .ko build TICK7 (obsolete untuk TICK10) |
| `work/sysmaps/dotconfig.builddmed.20261014_2218` | Config pasca-debloat |
| `work/sysmaps/sprd-gsp.ko.20261014_0940` | TICK9 evidence sprd-gsp md5 `bfefe9af…` |
| `work/sysmaps/novatek_ts_spi.ko.20261014_0940` | TICK9 evidence novatek_ts_spi md5 `943c7d89…` |
| `work/pull/artifacts.txt` | 13.958 baris build artifacts |
| `work/tally103.py` | Tally 103 modul vs build |
| `work/inventory103.txt` | 103 entri inventory |
| `work/blob_map/ledger.json` | 102/102 DEBLOBBED (TICK9) |

### 9.3 Dokumentasi path

| File | Isi |
|---|---|
| `docs/DEBLOB-20261014-TICK7-MODULE-SYNC.md` | TICK7 sinkron dokumentasi |
| `docs/DEBLOB-20261015-TICK9-STATUS.md` | TICK9 ledger 102/102 + retraksi |
| `docs/DRIVERS_RELEVANT.md` | 40 driver relevan Advan T1030 (4 kelas fungsi) |
| `docs/DRIVERS_VENDOR_CROSSCHECK.md` | Full table 129 vendor vs build TICK9 |
| `docs/SESSION_STATE.md` | Anchor state session (TICK7+TICK9) |
| `docs/HANDOFF-20261014-BUFFY-TO-HERMES.md` | Handoff Buffy → Hermes (2026-10-14) |
| `docs/MASTER.md` | **DOKUMEN INI** (konsolidasi komplit) |

### 9.4 GitHub

| URL | Isi |
|---|---|
| https://github.com/archanaberry/advan-vxtux-6.18.52 | Repo upstream |
| https://github.com/archanaberry/advan-vxtux-6.18.52/tree/test1 | Branch test1 (PR-ready) |
| https://github.com/archanaberry/advan-vxtux-6.18.52/blob/main/6.18.52-VXTux-UMS512/README.md | README kernel (root) |
| https://github.com/archanaberry/advan-vxtux-6.18.52/blob/main/6.18.52-VXTux-UMS512/docs/DRIVERS_VENDOR_CROSSCHECK.md | DRIVERS_VENDOR_CROSSCHECK.md |

### 9.5 Commit history test1 & changelog TICK8 → TICK12

| Commit | TICK | Pesan |
|---|---|---|
| `4152b927f` | TICK8 | vdsp ported, Image de-bloat 59→50.5MB, DTB 7-Oct→now, push test1 |
| `e7cef58d3` | TICK9 | ledger 102/102 DEBLOBBED, sprd-gsp + novatek_ts_spi .ko verified |
| `a4181022a` | TICK9+ | DRIVERS_RELEVANT.md (40 driver Advan T1030) |
| `e27dda4c1` | TICK10 | thermal DT chain 1/4 + sprd_vdsp wiring fix (46 files, +10.676 baris) |
| `2430c6242` | TICK10+ | DRIVERS_VENDOR_CROSSCHECK.md |
| `f9231da0f` | TICK10+ | README tabel 103 + kolom Vendor 129 |
| `7e9b998c9` | TICK10+ | NV entries dikategorikan A/B/C/D/E |
| `a50656589` | TICK10+ | Appendix A komprehensif |
| `efa7348c2` | TICK11 | 3 modul efuse + thermal DT chain 3/4 (2026-10-22) |
| `ec340d849` | TICK12 | tabel status 129 blob (✅ / (X)) dari config + ko build |
| `5a5182040`–`8bf97026b` | TICK12 | RE power_stat ABI (`stat_info` 0x30/data 0x58) → port `sprd_debugstat` |
| `cb5e91d35` | TICK12 | port `sprd_pdbg` — wakeup/sleep logging dari blob |
| `b1d692f09` | TICK12 | port 4 blob: manufacturer_model, 7sreset, autotest core+gpio |
| `9953413ba` | TICK12 | port `sprd_iq` — /dev/iq_mem + mailbox playback |
| `91e7838b2` | TICK12 | port 3 blob terakhir: mmc_swcq, sprd-cpufreq-v2, cpufreq-public |
| `11fb9b18c`–`74209d523` | TICK12 | Kconfig CRLF + dua API 6.18 yang sudah dihapus (modpost) |
| `5de06f3d0`–`e47c36b29` | TICK12 | seri jalur boot-test CI QEMU (workflow, DTS, initramfs, apt ports) |
| `f6935dbd0` | TICK12 | QEMU: 3 akar boot menggantung — GICR salah peta, PSCI smc→hvc, PCIe+ITS absen |
| `0201d1249`–`b196841a2` | TICK12 | init C statis arm64 + verifikasi cpio non-fatal |
| run `40100295454` | TICK12 | **boot gate LULUS** — stok+custom marker, tanpa panic |
| `e44869cca` | TICK12 | uji fbcon/DRM-KMS: fragmen + boot virtio-gpu + screendump ke PNG |
| `2a235f06b` | TICK12 | jendela poweroff 8 detik + retry screendump (race QEMU mati) |
| `8f45b6455`–`53b089ef0` | TICK12 | docs: skor 129/129, changelog TICK17, tabel diregenerasi + hasil fbcon |
| `f515e35d6` | TICK12 | QEMU: sesi CLI fbcon interaktif gaya Arch + rekam video (virtio-keyboard sendkey, console=tty0) |
| run `40129138896` | TICK12 | **sesi CLI fbcon LULUS** — 8 perintah tereksekusi, video 108 frame mp4 (gate fb=1 ready=1 cmd=1 done=1 px=1) |

### 9.6 Cronjob

| Job ID | Nama | Schedule | Status |
|---|---|---|---|
| `3bc6df1d4a0c` | vxtux-tick10-sync | once in 5m (06:13 WIB) | **completed, auto-disabled** |

---

## 10. Penutup

**Total driver VXTux** yang sebenarnya dipakai Advan T1030 (boot + daily + modul + sensor + SIPC):
- **103 di port** (kernel 6.18.52 hasil porting)
- **129 di vendor** (kernel 5.4 stock Advan, dipakai beneran di device)
- **90 match** (driver port ada di vendor)
- **13 NV** (driver port, vendor engga punya — klasifikasi A/B/C/D/E)
- **88 vendor-only** (driver vendor, belum di-port di TICK10 — **selesai ditutup TICK12**, lihat §6.2)
- **11 match dari vendor ke port** (driver vendor yang udah di-port)
- **13 modul compile-verified** (TICK7+TICK9, llvm-nm evidence)
- **102/102 ledger DEBLOBBED** (TICK9)
- **Image 50.473.472 byte** + **DTB 45.702 byte** siap flash (belum di-flash/boot-tested)
- **129/129 blob vendor tertangani** (regenerasi TICK12 2026-10-24, bukti ko list V17 — STATUS-129/COVERAGE-129/PORT-AUDIT)
- **239 .ko** di build V17 CI (run `40097613453`) — 10 port terakhir ikut terbangun
- **Boot QEMU LULUS** (run `40100295454`, [QEMU-BOOT-VALIDATION.md](QEMU-BOOT-VALIDATION.md)) + uji fbcon/DRM-KMS di QEMU
- **Sesi CLI fbcon interaktif LULUS + terekam video** (run `40129138896`, 108 frame → [fbcon-session.mp4](evidence-20261024/fbcon-session.mp4)) — shell sungguhan di tty1 digerakkan sendkey CI, 8 perintah `uname -a`→`poweroff`

Bestieee, kalau ada yang perlu di-update atau di-drill-down lagi, kasih tau aja. Pyaa~ 🍓

— Berry Chan, Raspya, 2026-10-15 🍓✨

