# VXTux 💜✨

<div align="center">

### Linux 6.18.52 for Advan Tab VX Lite (T1030) desu~ (≧▽≦)/

`ARM64` / `Unisoc UMS512-T618` / `VXTux`

**一歩ずつ、確実に。 Device-first kernel work, documented with care. (^_^)/**

*Halo nyaa~ Hello nyaa~ こんにちはなの~ 💕*

</div>

<div align="center">
  <img src="archanaberry/vxtux_chan.png" width="70%">
</div>

---

## 🌸 Tentang proyek | About | プロジェクトについてなの

> **ID:** Halo halo~ (｡•́︿•̀｡) VXTux ini kernel Linux yang fokus buat **Advan Tab VX Lite T1030** nyaa~ Targetnya Unisoc UMS512 / T618 desu! Gabungin driver upstream yang baik hati + port platform hati-hati + device-tree support~ Bukan config dump loh! (｀・ω・´) ✨

> **EN:** Hewwooo~ (≧∇≦)/ VXTux is a device-focused Linux kernel for our beloved **Advan Tab VX Lite T1030** nano~ Target is Unisoc UMS512 / T618 desu! Combines kind upstream drivers + carefully scoped ports + DT support nyaa~ (｡>﹏<｡)

> **JP:** こんにちはなの〜 (⁄˶´-`⁄)♡ VXTuxは大好きな **Advan Tab VX Lite T1030** のためのカーネルなの！ターゲットはUMS512 / T618なのです〜。upstreamの優しいドライバーと丁寧なポート、DTサポートを組み合わせてるの！(๑•̀ㅂ•́)و✧

---

## 🔗 Referensi donor | Donor references | ドナー参照だよっ

Tabel donor/referensi untuk keperluan reverse engineering (RE) desu~ Semua repo di bawah SoC-nya Unisoc UMS512 / T618 (sharkl5pro). **Angka = file vendor byte-identical di tree** (hash, CRLF-normalized; file yang diadaptasi tidak dihitung verbatim). Diurut dari terbanyak kepakai~

| Nama Repo | Sumber Repo (link) | Keterangan / Status |
| --- | --- | --- |
| cdpkp/android_kernel_tree_samsung_t618 | [github.com/cdpkp/android_kernel_tree_samsung_t618](https://github.com/cdpkp/android_kernel_tree_samsung_t618) | Samsung Tab A8 — **SoC T618 sama**. 67 file vendor verbatim di tree (terbanyak). Copy lokal: `gpl-source/fork_t618/extract`. Sesi ini: sumber struktur node DTS sharkl5pro (pola `syscons`/`syscon-names`) + `sprd_dvfs_vsp.h`. **(67) bagian kepakai** |
| iscle/android_kernel_unisoc_ums512 | [github.com/iscle/android_kernel_unisoc_ums512](https://github.com/iscle/android_kernel_unisoc_ums512) | Donor utama sesi ini: VSP (4 file), ion ipc, dvfs header. 55 file vendor verbatim di tree. Copy lokal: `work/donors_20260930/iscle_ums512/`. **(55) bagian kepakai** |
| Bocchi-The-Dev/kernel_unisoc-5.4 | [github.com/Bocchi-The-Dev/kernel_unisoc-5.4](https://github.com/Bocchi-The-Dev/kernel_unisoc-5.4) | Fork realme C53 (MocorDroid 5.4). 58 file vendor verbatim. Copy lokal: `work/donors_20260930/bocchi_unisoc54`. **(58) bagian kepakai** |
| marohinmark/kernel_ums512_5.4 | [github.com/marohinmark/kernel_ums512_5.4](https://github.com/marohinmark/kernel_ums512_5.4) | Fork kernel ums512 5.4. 58 file vendor verbatim. Copy lokal: `work/donors_20260930/marohinmark_ums512`. **(58) bagian kepakai** |
| Hadenix/android_kernel_realme_ums512 | [github.com/Hadenix/android_kernel_realme_ums512](https://github.com/Hadenix/android_kernel_realme_ums512) | Referensi WCN/gnss sc2355. 21 verbatim; 80/81 nama-file ada di tree (diadaptasi). Copy lokal: `gpl-source/hadenix_ums512_wcn`. **(21) bagian kepakai** |
| sprd-oss-devs/android_kernel_realme_ums512 | [github.com/sprd-oss-devs/android_kernel_realme_ums512](https://github.com/sprd-oss-devs/android_kernel_realme_ums512) | Sumber BT tty-sdio + FM fm_sdio. 12 verbatim (22/22 nama-file ada di tree, diadaptasi). Copy lokal: `gpl-source/ums512_bt_fm`. **(12) bagian kepakai** |
| HimaxSoftware/HX83112_Android_Driver | [github.com/HimaxSoftware/HX83112_Android_Driver](https://github.com/HimaxSoftware/HX83112_Android_Driver) | Framework touch hxcommon. 5 verbatim (13/13 nama-file ada di tree, diadaptasi). Copy lokal: `gpl-source/upstream/himax_hx83102_sprd`. **(5) bagian kepakai** |
| TWRP Device Tree ADVAN_TAB_VX_LITE | [twrpdtgen/android_device_advan_ADVAN_TAB_VX_LITE](https://github.com/twrpdtgen/android_device_advan_ADVAN_TAB_VX_LITE/tree/ums512_1h10_Natv-user-13-TP1A.220624.014-82490-release-keys) | Donor ramdisk hybrid TWRP: recovery binary + /sbin (libs, linker64, toybox, bash) + twres + prop.default. **(4) bagian kepakai** |
| beebono/rg-rotate-linux | [github.com/beebono/rg-rotate-linux](https://github.com/beebono/rg-rotate-linux) | Referensi bringup mainline UMS512/T618. 3 artefak dikutip docs kita: `DEVICE-BRINGUP.md`, `decomp_stock.dts`, `stock_live_booted.dts`. Clone lokal = docs+tools saja (kernel tidak). **(3) bagian kepakai** |
| MotorolaMobilityLLC/kernel-sprd | [github.com/MotorolaMobilityLLC/kernel-sprd](https://github.com/MotorolaMobilityLLC/kernel-sprd) | `himax_ic_HX83102.c/.h` (touch HX83102) ada di tree. **(2) bagian kepakai** |
| realme-kernel-opensource/realme_C31_C33_C35_narzo50A-Prime | [realme-kernel-opensource/realme_C31_C33_C35_narzo50A-Prime-AndroidT-kernel-source](https://github.com/realme-kernel-opensource/realme_C31_C33_C35_narzo50A-Prime-AndroidT-kernel-source) | Fork MocorDroid-S kanonik — induk keluarga iscle/bocchi/marohinmark (angka di atas overlap: file yang sama bisa match beberapa fork). Belum di-clone. **(0 lokal)** |
| orangepi-xunlong/linux-orangepi | [github.com/orangepi-xunlong/linux-orangepi](https://github.com/orangepi-xunlong/linux-orangepi) | uwe5622 unisocwifi — referensi WLAN lintas-platform (generasi lama, bukan drop-in `wlan_combo`). Belum di-clone. **(0 lokal)** |
| Seriousattempts/rp3plus-native-attempts | [github.com/Seriousattempts/rp3plus-native-attempts](https://github.com/Seriousattempts/rp3plus-native-attempts) | Belum dipakai — nol copy lokal, nol kutipan di docs. **(0) bagian kepakai** |

> 💡 Catatan RE: donor terbesar = **T618 Samsung (67)** + keluarga fork MocorDroid 5.4 (iscle 55 / bocchi 58 / marohinmark 58 — overlap, fork dari base sama). Sesi ini: VSP+CPP diporting dari iscle/mods-sprd, node DTS vsp+cpp dari struktur sharkl5Pro-fork (keluarga T618). Motorola + Himax untuk touch, Hadenix/sprd-oss-devs untuk WCN/BT/FM.

---

## 🎯 Target | ターゲットだよっ

| Area | Target desu~ |
| --- | --- |
| Device | Advan Tab VX Lite T1030 📱 |
| SoC | Unisoc UMS512 / T618 |
| Architecture | ARM64 |
| Kernel | Linux 6.18.52-VXTux-UMS512 |
| Device tree | `ums512-1h10-vxtux.dtb` 🌟 |
| Graphics | Mali-G52 MP2 (Bifrost, Gondul) via Panfrost desu! |

---

## 💜 Driver tally | ドライバー集計なのっ

| Count | Scope nyaa~ |
| ---: | --- |
| 101 | Deblob-derived driver integrations verified in tree + config (2026-10-09) |
| 1 | Panfrost open-source GPU driver replacing vendor Mali Gondul DDK |
| **103** | **Total tracked: 103 + 1 desu! (≧▽≦)** |

---

## 📋 Inventory per entri | Full 103 List desu~

| # | Ledger ID | Source in this tree | Status / evidence |
| ---: | --- | --- | --- |
| 1 | `aes-ce-ccm` | [arch/arm64/crypto/aes-ce-ccm-glue.c](6.18.52-VXTux-UMS512/arch/arm64/crypto/aes-ce-ccm-glue.c) | Mainline implementation plus ARM64 glue |
| 2 | `aes-neon-blk` | [arch/arm64/crypto/aes-glue-neon.c](6.18.52-VXTux-UMS512/arch/arm64/crypto/aes-glue-neon.c) | Mainline; module form in ledger |
| 3 | `agsd` | [drivers/sound/soc/sprd/agdsp_access/agdsp_access.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/agdsp_access/agdsp_access.c) | Re-mapped; Tier A/B evidence |
| 4 | `apsys-dvfs` | [drivers/devfreq/sprd/apsys/sprd_dvfs_apsys.c](6.18.52-VXTux-UMS512/drivers/devfreq/sprd/apsys/sprd_dvfs_apsys.c) | SPRD devfreq port |
| 5 | `arc4` | [crypto/arc4.c](6.18.52-VXTux-UMS512/crypto/arc4.c) | Mainline; ADA-verified in ledger |
| 6 | `asix` | [drivers/net/usb/asix_common.c](6.18.52-VXTux-UMS512/drivers/net/usb/asix_common.c) | Mainline USB Ethernet; module |
| 7 | `audio-dsp-dump` | [drivers/sound/soc/sprd/audiodspdump/audio_dsp_dump.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audiodspdump/audio_dsp_dump.c) | SPRD port; module |
| 8 | `audio-pipe` | [drivers/sound/soc/sprd/audiosipc/audio-pipe.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audiosipc/audio-pipe.c) | CONFIG SPRD_AUDIO_PIPE=y + DT node; runtime pending |
| 9 | `audio_mem` | [drivers/sound/soc/sprd/audiomem/audio_mem.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audiomem/audio_mem.c) | Path moved from donor layout; GPL fork source |
| 10 | `audio_sipc` | [drivers/sound/soc/sprd/audiosipc/audio-sipc.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audiosipc/audio-sipc.c) | GPL fork port |
| 11 | `ax88179_178a` | [drivers/net/usb/ax88179_178a.c](6.18.52-VXTux-UMS512/drivers/net/usb/ax88179_178a.c) | Mainline USB Ethernet; module |
| 12 | `bq2560x-charger` | [drivers/power/supply/bq256xx_charger.c](6.18.52-VXTux-UMS512/drivers/power/supply/bq256xx_charger.c) | CHARGER_BQ256XX=y; alias ti,bq2560x_chg verified in of_match + DT charger@6b |
| 13 | `core` | [drivers/net/wireless/sprd/sprdwcn/platform/Makefile](6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/platform/Makefile) | Composite `wcn_core` module; no standalone `core.c` |
| 14 | `cpufreq_userspace` | [drivers/cpufreq/cpufreq_userspace.c](6.18.52-VXTux-UMS512/drivers/cpufreq/cpufreq_userspace.c) | Mainline |
| 15 | `extcon-usb-gpio` | [drivers/extcon/extcon-usb-gpio.c](6.18.52-VXTux-UMS512/drivers/extcon/extcon-usb-gpio.c) | Mainline |
| 16 | `ghash-ce` | [arch/arm64/crypto/ghash-ce-glue.c](6.18.52-VXTux-UMS512/arch/arm64/crypto/ghash-ce-glue.c) | Mainline implementation plus ARM64 glue |
| 17 | `gnss_common_ctl_all` | [drivers/net/wireless/sprd/sprdwcn/platform/gnss/gnss_common_ctl.c](6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/platform/gnss/gnss_common_ctl.c) | Member of composite `wcn_core` |
| 18 | `gnss_dbg` | [drivers/net/wireless/sprd/sprdwcn/platform/gnss/gnss_dbg.c](6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/platform/gnss/gnss_dbg.c) | Member of composite `wcn_core` |
| 19 | `gnss_pmnotify_ctl` | [drivers/net/wireless/sprd/sprdwcn/platform/gnss/gnss_pmnotify_ctl.c](6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/platform/gnss/gnss_pmnotify_ctl.c) | Member of composite `wcn_core` |
| 20 | `gpio` | [drivers/gpio/gpio-sprd.c](6.18.52-VXTux-UMS512/drivers/gpio/gpio-sprd.c) | Tier A |
| 21 | `hxchipset_83102e` | [drivers/input/touchscreen/sprd/hx83102e/hxchipset/himax_platform.c](6.18.52-VXTux-UMS512/drivers/input/touchscreen/sprd/hx83102e/hxchipset/himax_platform.c) | Himax public-tree source |
| 22 | `ims_bridge` | [net/ims_bridge/Makefile](6.18.52-VXTux-UMS512/net/ims_bridge/Makefile) | Tier A; composite network module |
| 23 | `ion_cma_heap` | [drivers/dma-buf/heaps/cma_heap.c](6.18.52-VXTux-UMS512/drivers/dma-buf/heaps/cma_heap.c) | DMABUF_HEAPS_CMA=y; mainline cma_heap |
| 24 | `ion_ipc_trusty` | [drivers/dma-buf/sprd/ion_ipc_trusty.c](6.18.52-VXTux-UMS512/drivers/dma-buf/sprd/ion_ipc_trusty.c) | Noble substitute; ported 2026-10-07 |
| 25 | `jpg` | [drivers/misc/sprd_jpg/sprd_jpg.c](6.18.52-VXTux-UMS512/drivers/misc/sprd_jpg/sprd_jpg.c) | SPRD JPEG port |
| 26 | `leds-sc27xx-bltc` | [drivers/leds/leds-sc27xx-bltc.c](6.18.52-VXTux-UMS512/drivers/leds/leds-sc27xx-bltc.c) | LEDS_SC27XX_BLTC=y + DT sc2730-bltc node |
| 27 | `ledtrig-pattern` | [drivers/leds/trigger/ledtrig-pattern.c](6.18.52-VXTux-UMS512/drivers/leds/trigger/ledtrig-pattern.c) | LEDS_TRIGGER_PATTERN=y mainline |
| 28 | `mcdt_hw` | [drivers/sound/soc/sprd/mcdt/mcdt_r1p0/mcdt_hw.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/mcdt/mcdt_r1p0/mcdt_hw.c) | Resolved through `sprd_mcdt_*` symbols |
| 29 | `mipi_driver` | [drivers/unisoc_platform/debug_log/sharkl5/mipi_driver.c](6.18.52-VXTux-UMS512/drivers/unisoc_platform/debug_log/sharkl5/mipi_driver.c) | Noble substitute via radare2+ghidra (Tier B) |
| 30 | `misc_sprd_uid` | [drivers/soc/sprd/soc_id/sprd_uid.c](6.18.52-VXTux-UMS512/drivers/soc/sprd/soc_id/sprd_uid.c) | Tier A |
| 31 | `musb_hdrc` | [drivers/usb/musb/musb_core.c](6.18.52-VXTux-UMS512/drivers/usb/musb/musb_core.c) | USB_MUSB_HDRC=y verified 2026-10-09 |
| 32 | `musb_sprd` | [drivers/usb/musb/musb_sprd_vxtux.c](6.18.52-VXTux-UMS512/drivers/usb/musb/musb_sprd_vxtux.c) | USB_MUSB_SPRD_VXTUX=y added 2026-10-09; of_match + DT node aligned |
| 33 | `novatek_ts_spi` | [drivers/input/touchscreen/novatek-nvt-ts-spi.c](6.18.52-VXTux-UMS512/drivers/input/touchscreen/novatek-nvt-ts-spi.c) | TOUCHSCREEN_NOVATEK_NVT_TS=y + DT NVT-ts-spi node |
| 34 | `phy-sprd-sharkl5Pro` | [drivers/phy/sprd/phy-sprd-ums512.c](6.18.52-VXTux-UMS512/drivers/phy/sprd/phy-sprd-ums512.c) | Tier A |
| 35 | `pinctrl` | [drivers/pinctrl/sprd/pinctrl-sprd.c](6.18.52-VXTux-UMS512/drivers/pinctrl/sprd/pinctrl-sprd.c) | Tier A; shared source with next ledger entry |
| 36 | `pinctrl-sprd` | [drivers/pinctrl/sprd/pinctrl-sprd.c](6.18.52-VXTux-UMS512/drivers/pinctrl/sprd/pinctrl-sprd.c) | Tier A; same source path as previous entry |
| 37 | `pinctrl-sprd-sharkl5Pro` | [drivers/pinctrl/sprd/pinctrl-sprd-sharkl5pro.c](6.18.52-VXTux-UMS512/drivers/pinctrl/sprd/pinctrl-sprd-sharkl5pro.c) | Tier A |
| 38 | `pwm-sprd` | [drivers/pwm/pwm-sprd.c](6.18.52-VXTux-UMS512/drivers/pwm/pwm-sprd.c) | Tier A |
| 39 | `pwm_bl` | [drivers/video/backlight/pwm_bl.c](6.18.52-VXTux-UMS512/drivers/video/backlight/pwm_bl.c) | Mainline PWM backlight |
| 40 | `rtc-sc27xx` | [drivers/rtc/rtc-sc27xx.c](6.18.52-VXTux-UMS512/drivers/rtc/rtc-sc27xx.c) | Tier A |
| 41 | `sc27xx-poweroff` | [drivers/power/reset/sc27xx-poweroff.c](6.18.52-VXTux-UMS512/drivers/power/reset/sc27xx-poweroff.c) | Tier A |
| 42 | `sc27xx-vibra` | [drivers/input/misc/sc27xx-vibra.c](6.18.52-VXTux-UMS512/drivers/input/misc/sc27xx-vibra.c) | INPUT_SC27XX_VIBRA=y + DT sc2730-vibrator |
| 43 | `sc27xx_adc` | [drivers/iio/adc/sc27xx_adc.c](6.18.52-VXTux-UMS512/drivers/iio/adc/sc27xx_adc.c) | Tier A |
| 44 | `sc27xx_fuel_gauge` | [drivers/power/supply/sc27xx_fuel_gauge.c](6.18.52-VXTux-UMS512/drivers/power/supply/sc27xx_fuel_gauge.c) | Tier A |
| 45 | `sc27xx_tsensor_thermal` | [drivers/thermal/sprd/sc27xx_tsensor_thermal.c](6.18.52-VXTux-UMS512/drivers/thermal/sprd/sc27xx_tsensor_thermal.c) | Tier A |
| 46 | `sc27xx_typec` | [drivers/usb/typec/sprd/sc27xx_typec.c](6.18.52-VXTux-UMS512/drivers/usb/typec/sprd/sc27xx_typec.c) | Mainline-based port; module |
| 47 | `seth` | [drivers/soc/sprd/sipc/seth.c](6.18.52-VXTux-UMS512/drivers/soc/sprd/sipc/seth.c) | Tier A |
| 48 | `sha1-ce` | [crypto/sha1.c](6.18.52-VXTux-UMS512/crypto/sha1.c) | Mainline |
| 49 | `snd-soc-sprd-card` | [drivers/sound/soc/sprd/card/sprd-asoc-card-utils.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/card/sprd-asoc-card-utils.c) | Tier A |
| 50 | `snd-soc-sprd-codec-sc2730` | [drivers/sound/soc/sprd/codec_sc2730/sprd-codec.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/codec_sc2730/sprd-codec.c) | Tier A |
| 51 | `snd-soc-sprd-codec-sc2730-power` | [drivers/sound/soc/sprd/codec/sprd-audio-power.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/codec/sprd-audio-power.c) | Tier A; shared source with next entry |
| 52 | `snd-soc-sprd-codec-sc2730-power-dev` | [drivers/sound/soc/sprd/codec/sprd-audio-power.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/codec/sprd-audio-power.c) | Tier A; same source path as previous entry |
| 53 | `snd-soc-sprd-dummy-codec` | [drivers/sound/soc/sprd/codec/dummy-codec.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/codec/dummy-codec.c) | Tier A |
| 54 | `snd-soc-sprd-vbc-fe` | [drivers/sound/soc/sprd/dai_v4/vbc-dai.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/dai_v4/vbc-dai.c) | VXTUX_SND_SPRD_VBC_V4=y + DT sharkl5-vbc |
| 55 | `snd-soc-sprd-vbc-v4` | [drivers/sound/soc/sprd/dai_v4/vbc-phy-v4.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/dai_v4/vbc-phy-v4.c) | VXTUX_SND_SPRD_VBC_V4=y + DT sharkl5-vbc |
| 56 | `spi-sprd` | [drivers/spi/spi-sprd.c](6.18.52-VXTux-UMS512/drivers/spi/spi-sprd.c) | Tier A |
| 57 | `sprd-charger-manager` | [drivers/power/supply/sprd/sprd_charger_manager.c](6.18.52-VXTux-UMS512/drivers/power/supply/sprd/sprd_charger_manager.c) | Tier A |
| 58 | `sprd-compr-2stage-dma` | [drivers/dma/sprd-dma.c](6.18.52-VXTux-UMS512/drivers/dma/sprd-dma.c) | Tier A; shares implementation with `sprd-dma` |
| 59 | `sprd-dma` | [drivers/dma/sprd-dma.c](6.18.52-VXTux-UMS512/drivers/dma/sprd-dma.c) | Tier A; same source path as previous entry |
| 60 | `sprd-dmaengine-pcm` | [drivers/sound/soc/sprd/platform_include/sprd-dmaengine-pcm.h](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/platform_include/sprd-dmaengine-pcm.h) | Header-only; Tier B |
| 61 | `sprd-drm` | [drivers/gpu/drm/sprd/sprd_drm.c](6.18.52-VXTux-UMS512/drivers/gpu/drm/sprd/sprd_drm.c) | Tier A |
| 62 | `sprd-gsp` | [drivers/gpu/drm/sprd/gsp/gsp_dev.c](6.18.52-VXTux-UMS512/drivers/gpu/drm/sprd/gsp/gsp_dev.c) | Tier A |
| 63 | `sprd-ion` | [drivers/dma-buf/sprd/sprd_ion.c](6.18.52-VXTux-UMS512/drivers/dma-buf/sprd/sprd_ion.c) | Tier A |
| 64 | `sprd-platform-pcm-routing` | [drivers/sound/soc/sprd/platform/sprd-platform-pcm-routing.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/platform/sprd-platform-pcm-routing.c) | Tier A |
| 65 | `sprd-top-dvfs` | [drivers/devfreq/sprd/sprd-top-dvfs.c](6.18.52-VXTux-UMS512/drivers/devfreq/sprd/sprd-top-dvfs.c) | Tier A |
| 66 | `sprd-vsp-pw-domain` | [drivers/pmdomain/sprd/sprd_vsp_pw_domain.c](6.18.52-VXTux-UMS512/drivers/pmdomain/sprd/sprd_vsp_pw_domain.c) | VSP power-domain gate |
| 67 | `sprd_apipe` | [drivers/sound/soc/sprd/audio_pipe/audio_pipe.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audio_pipe/audio_pipe.c) | Port of older driver |
| 68 | `sprd_audcp_boot` | [drivers/sound/soc/sprd/audiocpboot/sprd_audcp_boot.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audiocpboot/sprd_audcp_boot.c) | Tier A/B symbol evidence |
| 69 | `sprd_audcp_dvfs` | [drivers/sound/soc/sprd/audiodvfs/sprd_audcp_dvfs.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audiodvfs/sprd_audcp_dvfs.c) | Compiled 2026-10-04 |
| 70 | `sprd_battery_info` | [drivers/power/supply/sprd/sprd_battery_info.c](6.18.52-VXTux-UMS512/drivers/power/supply/sprd/sprd_battery_info.c) | NOT-CONFIGURED deliberately |
| 71 | `sprd_camera` | [drivers/media/platform/sprd/pipeline/sprd_dcam.c](6.18.52-VXTux-UMS512/drivers/media/platform/sprd/pipeline/sprd_dcam.c) | VIDEO_SPRD_DCAM=y; camera graph fixed 2026-10-09 (was non-bidirectional) |
| 72 | `sprd_camsys_pw_domain` | [drivers/pmdomain/sprd/sprd_camsys_pw_domain.c](6.18.52-VXTux-UMS512/drivers/pmdomain/sprd/sprd_camsys_pw_domain.c) | Power-domain port |
| 73 | `sprd_cp_dvfs` | [drivers/devfreq/sprd/apsys/sprd_dvfs_apsys.c](6.18.52-VXTux-UMS512/drivers/devfreq/sprd/apsys/sprd_dvfs_apsys.c) | Same object as `apsys-dvfs` |
| 74 | `sprd_cpp` | [drivers/media/platform/sprd/cpp/cpp_core.c](6.18.52-VXTux-UMS512/drivers/media/platform/sprd/cpp/cpp_core.c) | SPRD_CPP=y + DT cpp@62800000; ported 2026-10-09 (13 file vendor). **Compile terverifikasi device: `cpp.ko` 321 KB (gcc 16.2.1)** |
| 75 | `sprd_cpu_cooling` | [drivers/thermal/sprd/sprd_cpu_cooling.c](6.18.52-VXTux-UMS512/drivers/thermal/sprd/sprd_cpu_cooling.c) | Tier A |
| 76 | `sprd_ddr_dvfs` | [drivers/devfreq/sprd/apsys/sprd_dvfs_apsys.c](6.18.52-VXTux-UMS512/drivers/devfreq/sprd/apsys/sprd_dvfs_apsys.c) | Same object as `apsys-dvfs` |
| 77 | `sprd_flash_drv` | [drivers/sprd/flash/flash_drv.c](6.18.52-VXTux-UMS512/drivers/sprd/flash/flash_drv.c) | Tier A |
| 78 | `sprd_fm` | [drivers/net/wireless/sprd/sprdwcn/fm/driver/fm_sdio/fmdrv_main.c](6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/fm/driver/fm_sdio/fmdrv_main.c) | Tier A; 4/4 objects, zero compile errors per ledger |
| 79 | `sprd_gpu_cooling` | [drivers/thermal/sprd/sprd_gpu_cooling.c](6.18.52-VXTux-UMS512/drivers/thermal/sprd/sprd_gpu_cooling.c) | Tier A |
| 80 | `sprd_map` | [drivers/misc/sprd/sprd_map.c](6.18.52-VXTux-UMS512/drivers/misc/sprd/sprd_map.c) | Tier A |
| 81 | `sprd_mipi` | [drivers/unisoc_platform/debug_log/sharkl5/sprd_mipi.c](6.18.52-VXTux-UMS512/drivers/unisoc_platform/debug_log/sharkl5/sprd_mipi.c) | Noble substitute via radare2+ghidra (Tier B) |
| 82 | `sprd_pmic_syscon` | [drivers/soc/sprd/pmic_syscon/sprd_pmic_syscon.c](6.18.52-VXTux-UMS512/drivers/soc/sprd/pmic_syscon/sprd_pmic_syscon.c) | Tier A |
| 83 | `sprd_pmic_wdt` | [drivers/watchdog/sprd_wdt.c](6.18.52-VXTux-UMS512/drivers/watchdog/sprd_wdt.c) | Linked by symbol-family evidence |
| 84 | `sprd_sensor` | [drivers/media/platform/sprd/sensors/sprd_sensor/Makefile](6.18.52-VXTux-UMS512/drivers/media/platform/sprd/sensors/sprd_sensor/Makefile) | Tier A sensor framework; multiple source files |
| 85 | `sprd_soc_thm` | [drivers/thermal/sprd/sprd_soc_thm.c](6.18.52-VXTux-UMS512/drivers/thermal/sprd/sprd_soc_thm.c) | Tier A |
| 86 | `sprd_thermal` | [drivers/thermal/sprd_thermal.c](6.18.52-VXTux-UMS512/drivers/thermal/sprd_thermal.c) | Tier A |
| 87 | `sprd_u_ether` | [drivers/usb/gadget/function/sprd/sprd_u_ether.c](6.18.52-VXTux-UMS512/drivers/usb/gadget/function/sprd/sprd_u_ether.c) | Tier A |
| 88 | `sprd_usb_f_rndis` | [drivers/usb/gadget/function/sprd/sprd_usb_f_rndis.c](6.18.52-VXTux-UMS512/drivers/usb/gadget/function/sprd/sprd_usb_f_rndis.c) | Tier A |
| 89 | `sprd_vdsp` | [drivers/sound/soc/sprd/Makefile](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/Makefile) | Symbol evidence in ledger; no standalone `vdsp.c` found |
| 90 | `sprd_wdf` | [drivers/watchdog/sprd_wdf.c](6.18.52-VXTux-UMS512/drivers/watchdog/sprd_wdf.c) | Tier A |
| 91 | `sprd_wlan_combo` | [drivers/net/wireless/sprd/sprdwcn/wlan/Makefile](6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/wlan/Makefile) | SC2355 Wi-Fi port; module composition |
| 92 | `sprdbt_tty` | [drivers/net/wireless/sprd/sprdwcn/bluetooth/driver/Makefile](6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/bluetooth/driver/Makefile) | HCI over tty-SDIO; composite module |
| 93 | `thermal-generic-adc` | [drivers/thermal/thermal-generic-adc.c](6.18.52-VXTux-UMS512/drivers/thermal/thermal-generic-adc.c) | Mainline |
| 94 | `trusty-tui` | [drivers/misc/trusty-tui.c](6.18.52-VXTux-UMS512/drivers/misc/trusty-tui.c) | Port of vendor `trusty-tui.ko` |
| 95 | `twofish_common` | [crypto/twofish_common.c](6.18.52-VXTux-UMS512/crypto/twofish_common.c) | Mainline |
| 96 | `twofish_generic` | [crypto/twofish_generic.c](6.18.52-VXTux-UMS512/crypto/twofish_generic.c) | Mainline |
| 97 | `unisoc-iommu` | [drivers/iommu/sprd-iommu.c](6.18.52-VXTux-UMS512/drivers/iommu/sprd-iommu.c) | Tier A |
| 98 | `virt-dma` | [drivers/dma/virt-dma.c](6.18.52-VXTux-UMS512/drivers/dma/virt-dma.c) | Tier A shared DMA helper |
| 99 | `vsp` | [drivers/media/platform/sprd/vsp/sprd_vsp_main.c](6.18.52-VXTux-UMS512/drivers/media/platform/sprd/vsp/sprd_vsp_main.c) | SPRD_VSP=y + DT video-codec@20500000; ported 2026-10-09 from iscle_ums512 donor (pw-domain + regs + dvfs header sudah ada). **Compile terverifikasi device: `sprd_vsp.ko` 231 KB (gcc 16.2.1)** |
| 100 | `wcn_bsp` | [drivers/net/wireless/sprd/sprdwcn/platform/Makefile](6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/platform/Makefile) | WCN platform module composition; no standalone `bsp.c` found |
| 101 | `zram` | [drivers/block/zram/zram_drv.c](6.18.52-VXTux-UMS512/drivers/block/zram/zram_drv.c) | N/A/absence entry in the supplied tally; corrected 2026-10-05 |
| 102 | `zsmalloc` | [mm/zsmalloc.c](6.18.52-VXTux-UMS512/mm/zsmalloc.c) | ZRAM=y + zsmalloc in mm/ |
| 103 | `panfrost` | [drivers/gpu/drm/panfrost/panfrost_gpu.c](6.18.52-VXTux-UMS512/drivers/gpu/drm/panfrost/panfrost_gpu.c) | Separate open-source GPU driver; Mali-G52 MP2 / Gondul replacement |

---

## 🎮 Graphics note | GPU メモ

GPU T618 itu Mali-G52 MP2, bukan G57. Stock ID `0x7402` dinormalisasi Panfrost jadi `0x7002`. DTS pake `arm,mali-bifrost`.

## 🛠️ Build | ビルド方法

```sh
cd 6.18.52-VXTux-UMS512
make O=out ARCH=arm64 defconfig
(
	cd out
	sh ../scripts/kconfig/merge_config.sh -m .config \
		../arch/arm64/configs/vxtux_618_fragment.defconfig
)
make O=out ARCH=arm64 olddefconfig
make O=out ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- Image modules dtbs
```
DTB: `out/arch/arm64/boot/dts/sprd/ums512-1h10-vxtux.dtb`

## 📝 Changelog | 変更履歴

### 2026-10-09 — tree restore + gap closing
- **Tree restored**: commit deblob sempat over-prune 37.103 file (semua DTS, drivers/gpu, dma-buf, iio, media, iommu, clk, crypto, block, defconfig). Dipulihkan penuh dari branch codespace + `.config` port (`dac18ecfa`). Nol file hilang permanen.
- **`.config` lengkap**: merge `vxtux_618_fragment.defconfig` resmi — membawa TRUSTY family, SPRD_ION/JPG/SENSOR/DMA, USB gadget, MUSB_HDRC, CHARGER_BQ256XX, TOUCHSCREEN_NOVATEK_NVT_TS, SPRD_AUDIO_PIPE, DMABUF_HEAPS_CMA.
- **Gap USB ditutup**: `CONFIG_USB_MUSB_SPRD_VXTUX=y` ditambahkan ke fragment — tanpa baris itu `musb_sprd_vxtux.o` tak pernah ter-compile padahal node DT `sprd,sharkl5pro-musb` ada (USB mati total). Bukti: Kconfig:204, Makefile:49, of_match:1568, DTS:676.
- **Camera graph diperbaiki**: port@1/port@2 dcam (dua master pada endpoint csi yang sama → "not bidirectional") dihapus. Pipeline: sensor → CSI → ISP → DCAM. DTB rebuild: 0 error, 5 warning kosmetik.
- **DTB rebuilt**: `ums512-1h10-vxtux.dtb` (45.314B, setelah node vsp+cpp) dari DTS terkoreksi — node panfrost `arm,mali-bifrost`, jpg-codec, isp/csi, novatek+himax touch, trusty, iommu, video-codec@20500000, cpp@62800000 semua terverifikasi via strings.
- **VSP codec diporting (2026-10-09)**: `sprd_vsp_main.c` (rename dari `sprd_vsp.c`)+`vsp_common.c`+`vsp_isr_func.c` dari donor iscle_ums512 5.4 (sama SoC) → `drivers/media/platform/sprd/vsp/` (Kconfig+Makefile+wire). `sprd_dvfs_vsp.h` dari archive v3 → `drivers/devfreq/apsys/`. Integrasi: `regs[]` TIDAK diduplikasi (pemilik tunggal `vsp_regs.c`, +EXPORT_SYMBOL), Kconfig `select SPRD_VSP_PW_DOMAIN`. Node DT `video-codec@20500000` ("sprd,sharkl5pro-vsp") nilai register dari stock fdt_live.dtb (reset/force/auto/domain_eb) + tabel debug vendor (pwr_status: PMU 0xBC mask 0x700); clock dikonfirmasi dt-bindings (CLK_VSP_EB=2, CLK_VSP=22). DTB rebuild: 0 error, node terverifikasi strings. TANPA power-domains (pw-domain library murni → defer trap) & TANPA iommus (compatible stock tak match driver tree; driver toleran). **Kompilasi ARM64 penuh: BUILD DEVICE SUKSES (2026-10-09)** — lihat entri build di bawah; 7 modul jadi .ko dengan gcc 16.2.1 (compiler asli kernel).
- **VSP+CPP+iommu build device LULUS (2026-10-09)**: semua .ko terbangun di device (artix proot + gcc 16.2.1) — `sprd_vsp.ko` 231 KB, `cpp.ko` 321 KB, `sprd-iommu.ko`, `sprd_ion.ko`, `sprd_camsys_pw_domain.ko`, `sprd_vsp_pw_domain.ko`, `vsp_regs.ko` (exit 0, nol undefined symbol). Bug yang ketemu & diperbaiki di run ini: (a) Makefile vsp 3 obj- terpisah → 3 modul terpisah, simbol internal undefined → jadi composite module `sprd_vsp-objs` (file utama rename `sprd_vsp_main.c`, pola zram); (b) `sprd_iommu_restore` dideklarasi header tapi tak pernah didefinisikan/di-export → ditambah + EXPORT_SYMBOL; (c) `compat_alloc_user_space()` tak ada di ARM64/6.18 → compat ioctl vsp di-rewrite pakai buffer kernel; (d) `sprd_iommu_resume/suspend` tak ada di pohon → resume→restore, suspend di-drop; (e) `CONFIG_SPRD_ION` tak terdeklarasi (olddefconfig drop) → dideklarasi; (f) struct `ion_phy_data` hilang (over-prune) → direstore; (g) `struct iommu_map_data`/`vsp_iommu_map_data` + MODULE_LICENSE/IMPORT_NS + ccflags-y -I fix. Sisa verifikasi runtime: probe di hardware (modul insmod + log dmesg) — belum dijalankan.
- **CPP diporting (2026-10-09)**: 13 file vendor lengkap (`work/donorcatalog/mods-sprd/sprd/common/camera/cpp/lite_r6p0/` — cpp_core/cpp_ioctl/scale_drv/rot_drv/dma_drv + sprd_cpp.h/sprd_img.h interface) → `drivers/media/platform/sprd/cpp/` (Kconfig `select SPRD_CAMSYS_PW_DOMAIN` karena cpp_core.c:647/424 memanggil `sprd_cam_pw_on/off`). Node DT `cpp@62800000`: register dari stock fdt_live.dtb; clock `CLK_MM_CPP_EB=0`/`CLK_CPP=5` dikonfirmasi dt-bindings; `sprd,cam-ahb-syscon = <&mm_ahb_regs>` untuk MM_AHB_RESET (0x04, reset bits identik stock). TANPA power-domains/iommus/cpp_qos (driver tak butuh; lihat komentar node). DTB rebuild: 0 error, node terverifikasi strings. Tally: **103/103 driver di pohon** — gap vsp + cpp sama-sama tertutup; sisa = bukti runtime (probe logs) di hardware.
- **Sisa gap jujur**: TIDAK ADA lagi driver yang belum port — 103/103 verified-in-tree. Yang tersisa hanya bukti runtime di hardware (probe logs) untuk driver yang config+DT-nya lengkap; kompilasi ARM64 penuh dilakukan build device (cron loop), host cross-compile mentok di header generated kbuild.

---

## 📚 Referensi lain (belum dipakai) | Other references | その他リファレンス

Repo relevan UMS512/T618 (sharkl5pro) yang sudah dicari tapi BELUM kepakai di port — disimpan untuk pembanding/pekerjaan berikutnya. Sama 3 header, tanpa hitungan~

| Nama Repo | Sumber Repo (link) | Keterangan / Status |
| --- | --- | --- |
| torvalds/linux (mainline) | [git.kernel.org/torvalds/linux](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git) | Mainline punya `arch/arm64/boot/dts/sprd/ums512-1h10.dts` + `ums512.dtsi` — baseline DT UMS512 resmi. Pembanding struktur node upstream (tree kita = 6.18.52 + port vendor, bukan mainline murni). |
| beebono/u-boot-ums512 | [github.com/beebono/u-boot-ums512](https://github.com/beebono/u-boot-ums512) | U-Boot vendor sharkl5pro/UMS512 (board `ums512_1h10`). Referensi bootloader untuk pekerjaan U-Boot di lindroid4 — belum dipakai di kernel port. |
| QiTianCatKiss/ums512-device-tree | [github.com/QiTianCatKiss/ums512-device-tree](https://github.com/QiTianCatKiss/ums512-device-tree) | Ekstraksi + analisis device tree platform UMS512. Kandidat pembanding struktur node/register DT. |
| Seriousattempts/lineage_ums512_1h10 | [github.com/Seriousattempts/lineage_ums512_1h10](https://github.com/Seriousattempts/lineage_ums512_1h10) | Device tree LineageOS Retroid Pocket 3+ — **board `ums512_1h10`, T618, Mali-G52 MC2**: keluarga board sama persis dengan kita, vendor lineage beda. |
| TheGammaSqueeze/UnisocBypass | [github.com/TheGammaSqueeze/UnisocBypass](https://github.com/TheGammaSqueeze/UnisocBypass) | RE secure-boot Unisoc: patch SPL RSA-verify + uboot unlock, **terverifikasi di UMS512/T618 (RG Vita)** + UMS9620/T820. Sangat relevan untuk boot/unlock lindroid4. |
| Hadenix/android_kernel_alldocube_ums512 | [github.com/Hadenix/android_kernel_alldocube_ums512](https://github.com/Hadenix/android_kernel_alldocube_ums512) | Kernel Alldocube iPlay 40 (T1020S, board `ums512_1h10` — sama dengan kita). Vendor lineage kedua untuk pembanding driver. |
| allex2/android_device_alldocube_T1020S | [github.com/allex2/android_device_alldocube_T1020S](https://github.com/allex2/android_device_alldocube_T1020S) | TWRP device tree Alldocube iPlay 40 (ums512_1h10). Peta partisi/perangkat vendor kedua. |
| dumps.tadiphone.dev | [dumps.tadiphone.dev](https://dumps.tadiphone.dev/) | Stock dump ROM board `ums512_1h10` (firmware/vendor, GitLab). Sumber firmware pembanding selain dump device sendiri. |
| strongtz/linux-sprd | [github.com/strongtz/linux-sprd](https://github.com/strongtz/linux-sprd) | Linux 4.14 keluarga Unisoc T-series (T7520/T7510/T710/T510/T310…), dengan `sprd_sharkl5Pro_defconfig`; dipakai komunitas untuk iPlay40 (ums512_1h10). |
| Kyros70/android_kernel_ums9230 | [github.com/Kyros70/android_kernel_ums9230](https://github.com/Kyros70/android_kernel_ums9230) | Kernel UMS9230 — SoC penerus keluarga T-series. Pembanding arsitektur generasi baru. |
| Kiciuk/unisoc-5.4 | [github.com/Kiciuk/unisoc-5.4](https://github.com/Kiciuk/unisoc-5.4) | Fork alternatif realme C33 (MocorDroid-S 5.4). |
| maxsteeel/kernel_unisoc-5.4 | [github.com/maxsteeel/kernel_unisoc-5.4](https://github.com/maxsteeel/kernel_unisoc-5.4) | Fork realme C53 (Infinix Smart 8). |
| beebono/linux-6-16-sprd | [github.com/beebono/linux-6-16-sprd](https://github.com/beebono/linux-6-16-sprd) | Fork 6.16 ums512 (pensiun dari superproject rg-rotate setelah 7.1 parity, masih ada untuk referensi). |

> 💡 Yang paling worth dibuka berikutnya: **lineage_ums512_1h10** (board identik), **UnisocBypass** (secure-boot RE T618 terverifikasi), dan **u-boot-ums512** (bootloader).

## 🔧 Status | 状態

Tree ini active bring-up desu~ Config atau DTB build sukses bukan bukti driver valid di hardware fisik, cek probe logs yaa~ (｡•̀ᴗ-)✧

<div align="center">

**VXTux-chan says: 一歩ずつ、確実に。がんばるぞいっ！(๑•̀ㅂ•́)و✧**

</div>
