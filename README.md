# VXTux 💜✨

<div align="center">

### Linux 6.18.52 for Advan Tab VX Lite (T1030) desu~ (≧▽≦)/

`ARM64` / `Unisoc UMS512-T618` / `VXTux`

**一歩ずつ、確実に。 Device-first kernel work, documented with care. (^_^)/**

*Halo nyaa~ Hello nyaa~ こんにちはなの~ 💕*

</div>

---
<div align="center">
  <img src="archanaberry/vxtux_chan.png" width="50%">
</div>
---

## 🌸 Tentang proyek | About | プロジェクトについてなの

> **ID:** Halo halo~ (｡•́︿•̀｡) VXTux ini kernel Linux yang fokus buat **Advan Tab VX Lite T1030** nyaa~ Targetnya Unisoc UMS512 / T618 desu! Gabungin driver upstream yang baik hati + port platform hati-hati + device-tree support~ Bukan config dump loh! (｀・ω・´) Semua pilihan hardware nyambung ke DTS target, Kconfig, sama build wiring nyaa~ ✨

> **EN:** Hewwooo~ (≧∇≦)/ VXTux is a device-focused Linux kernel for our beloved **Advan Tab VX Lite T1030** nano~ Target is Unisoc UMS512 / T618 desu! Combines kind upstream drivers + carefully scoped ports + DT support nya~ Not just config dump okay? (｡>﹏<｡)

> **JP:** こんにちはなの〜 (⁄˶´-`⁄)♡ VXTuxは大好きな **Advan Tab VX Lite T1030** のためのカーネルなの！ターゲットはUMS512 / T618なのです〜。upstreamの優しいドライバーと丁寧なポート、DTサポートを組み合わせてるの！config dumpじゃないんだからねっ！(๑•̀ㅂ•́)و✧

## 🎯 Target | ターゲットだよっ

| Area | Target desu~ |
| --- | --- |
| Device | Advan Tab VX Lite T1030 📱 |
| SoC | Unisoc UMS512 / T618 |
| Architecture | ARM64 |
| Kernel | Linux 6.18.52-VXTux-UMS512 |
| Device tree | `ums512-1h10-vxtux.dtb` 🌟 |
| Graphics | Mali-G52 MP2 (Bifrost, Gondul) via Panfrost desu! |

## 💜 Driver tally | ドライバー集計なのっ

| Count | Scope nyaa~ |
| ---: | --- |
| 102 | Deblob-derived driver integrations (｡•̀ᴗ-)✧ |
| 1 | Panfrost open-source GPU driver replacing vendor Mali Gondul DDK |
| **103** | **Total tracked: 102 + 1 desu! (≧▽≦)** |

Panfrost dihitung terpisah, ini hitungan integrasi bukan model GPU yaa~ Angka 102 itu project tally, bukan list per-driver reproducible di repo ini.

### 📋 Inventory per entri | Full 103 List desu~

<details>
<summary><b>✨ Click to expand 103 identifiers (102 + Panfrost) ✨</b></summary>

| # | Ledger ID | Source in this tree | Status |
| ---: | --- | --- | --- |
| 1 | `aes-ce-ccm` | `arch/arm64/crypto/aes-ce-ccm-glue.c` | Mainline + ARM64 glue |
| 2 | `aes-neon-blk` | `arch/arm64/crypto/aes-glue-neon.c` | Mainline; module |
| 3 | `agsd` | `drivers/sound/soc/sprd/agdsp_access/agdsp_access.c` | Re-mapped; Tier A/B |
| 4 | `apsys-dvfs` | `drivers/devfreq/sprd/apsys/sprd_dvfs_apsys.c` | SPRD devfreq port |
| 5 | `arc4` | `crypto/arc4.c` | Mainline |
| 6 | `asix` | `drivers/net/usb/asix_common.c` | Mainline USB Eth; module |
| 7 | `audio-dsp-dump` | `drivers/sound/soc/sprd/audiodspdump/audio_dsp_dump.c` | SPRD port; module |
| 8 | `audio-pipe` | `drivers/sound/soc/sprd/audiosipc/audio-pipe.c` | UNPROVEN |
| 9 | `audio_mem` | `drivers/sound/soc/sprd/audiomem/audio_mem.c` | GPL fork |
| 10 | `audio_sipc` | `drivers/sound/soc/sprd/audiosipc/audio-sipc.c` | GPL fork port |
| 11 | `ax88179_178a` | `drivers/net/usb/ax88179_178a.c` | Mainline USB Eth; module |
| 12 | `bq2560x-charger` | `drivers/power/supply/bq256xx_charger.c` | UNPROVEN |
| 13 | `core` | `drivers/net/wireless/sprd/sprdwcn/platform/Makefile` | Composite `wcn_core` |
| 14 | `cpufreq_userspace` | `drivers/cpufreq/cpufreq_userspace.c` | Mainline |
| 15 | `extcon-usb-gpio` | `drivers/extcon/extcon-usb-gpio.c` | Mainline |
| 16 | `ghash-ce` | `arch/arm64/crypto/ghash-ce-glue.c` | Mainline + glue |
| 17 | `gnss_common_ctl_all` | `drivers/net/wireless/sprd/sprdwcn/platform/gnss/gnss_common_ctl.c` | Member wcn_core |
| 18 | `gnss_dbg` | `drivers/net/wireless/sprd/sprdwcn/platform/gnss/gnss_dbg.c` | Member wcn_core |
| 19 | `gnss_pmnotify_ctl` | `drivers/net/wireless/sprd/sprdwcn/platform/gnss/gnss_pmnotify_ctl.c` | Member wcn_core |
| 20 | `gpio` | `drivers/gpio/gpio-sprd.c` | Tier A |
| 21 | `hxchipset_83102e` | `drivers/input/touchscreen/sprd/hx83102e/hxchipset/himax_platform.c` | Himax source |
| 22 | `ims_bridge` | `net/ims_bridge/Makefile` | Tier A composite |
| 23 | `ion_cma_heap` | `drivers/dma-buf/heaps/cma_heap.c` | UNPROVEN |
| 24 | `ion_ipc_trusty` | `drivers/dma-buf/sprd/ion_ipc_trusty.c` | Noble substitute 2026-10-07 |
| 25 | `jpg` | `drivers/misc/sprd_jpg/sprd_jpg.c` | SPRD JPEG port |
| 26 | `leds-sc27xx-bltc` | `drivers/leds/leds-sc27xx-bltc.c` | UNPROVEN |
| 27 | `ledtrig-pattern` | `drivers/leds/trigger/ledtrig-pattern.c` | UNPROVEN |
| 28 | `mcdt_hw` | `drivers/sound/soc/sprd/mcdt/mcdt_r1p0/mcdt_hw.c` | sprd_mcdt_* symbols |
| 29 | `mipi_driver` | `drivers/unisoc_platform/debug_log/sharkl5/` (absent) | Noble substitute; path absent |
| 30 | `misc_sprd_uid` | `drivers/soc/sprd/soc_id/sprd_uid.c` | Tier A |
| 31 | `musb_hdrc` | `drivers/usb/musb/musb_core.c` | Noble substitute |
| 32 | `musb_sprd` | `drivers/usb/musb/musb_sprd_vxtux.c` | Noble substitute partial |
| 33 | `novatek_ts_spi` | `drivers/input/touchscreen/novatek-nvt-ts-spi.c` | Noble substitute |
| 34 | `phy-sprd-sharkl5Pro` | `drivers/phy/sprd/phy-sprd-ums512.c` | Tier A |
| 35 | `pinctrl` | `drivers/pinctrl/sprd/pinctrl-sprd.c` | Tier A shared |
| 36 | `pinctrl-sprd` | `drivers/pinctrl/sprd/pinctrl-sprd.c` | Tier A same path |
| 37 | `pinctrl-sprd-sharkl5Pro` | `drivers/pinctrl/sprd/pinctrl-sprd-sharkl5pro.c` | Tier A |
| 38 | `pwm-sprd` | `drivers/pwm/pwm-sprd.c` | Tier A |
| 39 | `pwm_bl` | `drivers/video/backlight/pwm_bl.c` | Mainline |
| 40 | `rtc-sc27xx` | `drivers/rtc/rtc-sc27xx.c` | Tier A |
| 41 | `sc27xx-poweroff` | `drivers/power/reset/sc27xx-poweroff.c` | Tier A |
| 42 | `sc27xx-vibra` | `drivers/input/misc/sc27xx-vibra.c` | UNPROVEN |
| 43 | `sc27xx_adc` | `drivers/iio/adc/sc27xx_adc.c` | Tier A |
| 44 | `sc27xx_fuel_gauge` | `drivers/power/supply/sc27xx_fuel_gauge.c` | Tier A |
| 45 | `sc27xx_tsensor_thermal` | `drivers/thermal/sprd/sc27xx_tsensor_thermal.c` | Tier A |
| 46 | `sc27xx_typec` | `drivers/usb/typec/sprd/sc27xx_typec.c` | Mainline port module |
| 47 | `seth` | `drivers/soc/sprd/sipc/seth.c` | Tier A |
| 48 | `sha1-ce` | `crypto/sha1.c` | Mainline |
| 49 | `snd-soc-sprd-card` | `drivers/sound/soc/sprd/card/sprd-asoc-card-utils.c` | Tier A |
| 50 | `snd-soc-sprd-codec-sc2730` | `drivers/sound/soc/sprd/codec_sc2730/sprd-codec.c` | Tier A |
| 51 | `snd-soc-sprd-codec-sc2730-power` | `drivers/sound/soc/sprd/codec/sprd-audio-power.c` | Tier A shared |
| 52 | `snd-soc-sprd-codec-sc2730-power-dev` | `drivers/sound/soc/sprd/codec/sprd-audio-power.c` | Tier A same path |
| 53 | `snd-soc-sprd-dummy-codec` | `drivers/sound/soc/sprd/codec/dummy-codec.c` | Tier A |
| 54 | `snd-soc-sprd-vbc-fe` | `drivers/sound/soc/sprd/dai_v4/vbc-dai.c` | UNPROVEN |
| 55 | `snd-soc-sprd-vbc-v4` | `drivers/sound/soc/sprd/dai_v4/vbc-phy-v4.c` | UNPROVEN |
| 56 | `spi-sprd` | `drivers/spi/spi-sprd.c` | Tier A |
| 57 | `sprd-charger-manager` | `drivers/power/supply/sprd/sprd_charger_manager.c` | Tier A |
| 58 | `sprd-compr-2stage-dma` | `drivers/dma/sprd-dma.c` | Tier A shared |
| 59 | `sprd-dma` | `drivers/dma/sprd-dma.c` | Tier A same path |
| 60 | `sprd-dmaengine-pcm` | `drivers/sound/soc/sprd/platform_include/sprd-dmaengine-pcm.h` | Header-only Tier B |
| 61 | `sprd-drm` | `drivers/gpu/drm/sprd/sprd_drm.c` | Tier A |
| 62 | `sprd-gsp` | `drivers/gpu/drm/sprd/gsp/gsp_dev.c` | Tier A |
| 63 | `sprd-ion` | `drivers/dma-buf/sprd/sprd_ion.c` | Tier A |
| 64 | `sprd-platform-pcm-routing` | `drivers/sound/soc/sprd/platform/sprd-platform-pcm-routing.c` | Tier A |
| 65 | `sprd-top-dvfs` | `drivers/devfreq/sprd/sprd-top-dvfs.c` | Tier A |
| 66 | `sprd-vsp-pw-domain` | `drivers/pmdomain/sprd/sprd_vsp_pw_domain.c` | VSP gate |
| 67 | `sprd_apipe` | `drivers/sound/soc/sprd/audio_pipe/audio_pipe.c` | Port older |
| 68 | `sprd_audcp_boot` | `drivers/sound/soc/sprd/audiocpboot/sprd_audcp_boot.c` | Tier A/B |
| 69 | `sprd_audcp_dvfs` | `drivers/sound/soc/sprd/audiodvfs/sprd_audcp_dvfs.c` | Compiled 2026-10-04 |
| 70 | `sprd_battery_info` | `drivers/power/supply/sprd/sprd_battery_info.c` | NOT-CONFIGURED |
| 71 | `sprd_camera` | `drivers/media/platform/sprd/pipeline/sprd_dcam.c` | UNPROVEN |
| 72 | `sprd_camsys_pw_domain` | `drivers/pmdomain/sprd/sprd_camsys_pw_domain.c` | Power-domain port |
| 73 | `sprd_cp_dvfs` | `drivers/devfreq/sprd/apsys/sprd_dvfs_apsys.c` | Same as apsys-dvfs |
| 74 | `sprd_cpp` | `drivers/media/platform/sprd/pipeline/sprd_isp.c` | UNPROVEN |
| 75 | `sprd_cpu_cooling` | `drivers/thermal/sprd/sprd_cpu_cooling.c` | Tier A |
| 76 | `sprd_ddr_dvfs` | `drivers/devfreq/sprd/apsys/sprd_dvfs_apsys.c` | Same as apsys-dvfs |
| 77 | `sprd_flash_drv` | `drivers/sprd/flash/flash_drv.c` | Tier A |
| 78 | `sprd_fm` | `drivers/net/wireless/sprd/sprdwcn/fm/driver/fm_sdio/fmdrv_main.c` | Tier A 4/4 objects |
| 79 | `sprd_gpu_cooling` | `drivers/thermal/sprd/sprd_gpu_cooling.c` | Tier A |
| 80 | `sprd_map` | `drivers/misc/sprd/sprd_map.c` | Tier A |
| 81 | `sprd_mipi` | `drivers/unisoc_platform/debug_log/` (absent) | UNPROVEN |
| 82 | `sprd_pmic_syscon` | `drivers/soc/sprd/pmic_syscon/sprd_pmic_syscon.c` | Tier A |
| 83 | `sprd_pmic_wdt` | `drivers/watchdog/sprd_wdt.c` | Symbol-family |
| 84 | `sprd_sensor` | `drivers/media/platform/sprd/sensors/sprd_sensor/Makefile` | Tier A framework |
| 85 | `sprd_soc_thm` | `drivers/thermal/sprd/sprd_soc_thm.c` | Tier A |
| 86 | `sprd_thermal` | `drivers/thermal/sprd_thermal.c` | Tier A |
| 87 | `sprd_u_ether` | `drivers/usb/gadget/function/sprd/sprd_u_ether.c` | Tier A |
| 88 | `sprd_usb_f_rndis` | `drivers/usb/gadget/function/sprd/sprd_usb_f_rndis.c` | Tier A |
| 89 | `sprd_vdsp` | `drivers/sound/soc/sprd/Makefile` | Symbol evidence |
| 90 | `sprd_wdf` | `drivers/watchdog/sprd_wdf.c` | Tier A |
| 91 | `sprd_wlan_combo` | `drivers/net/wireless/sprd/sprdwcn/wlan/Makefile` | SC2355 Wi-Fi |
| 92 | `sprdbt_tty` | `drivers/net/wireless/sprd/sprdwcn/bluetooth/driver/Makefile` | HCI tty-SDIO |
| 93 | `thermal-generic-adc` | `drivers/thermal/thermal-generic-adc.c` | Mainline |
| 94 | `trusty-tui` | `drivers/misc/trusty-tui.c` | Port vendor ko |
| 95 | `twofish_common` | `crypto/twofish_common.c` | Mainline |
| 96 | `twofish_generic` | `crypto/twofish_generic.c` | Mainline |
| 97 | `unisoc-iommu` | `drivers/iommu/sprd-iommu.c` | Tier A |
| 98 | `virt-dma` | `drivers/dma/virt-dma.c` | Tier A helper |
| 99 | `vsp` | `drivers/pmdomain/sprd/vsp_regs.c` | VSP domain |
| 100 | `wcn_bsp` | `drivers/net/wireless/sprd/sprdwcn/platform/Makefile` | WCN platform |
| 101 | `zram` | `drivers/block/zram/zram_drv.c` | N/A corrected 2026-10-05 |
| 102 | `zsmalloc` | `mm/zsmalloc.c` | UNPROVEN corrected |
| 103 | `panfrost` | `drivers/gpu/drm/panfrost/panfrost_gpu.c` | Open-source GPU Mali-G52 MP2 replacement desu! ✨ |

</details>

**Count:** 102 deblob + 1 Panfrost = **103 entries** nyaa~ (≧▽≦)

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

## 🔧 Status | 状態

Tree ini active bring-up desu~ Config atau DTB build sukses bukan bukti driver valid di hardware fisik, cek probe logs yaa~ (｡•̀ᴗ-)✧

<div align="center">

**VXTux-chan says: 一歩ずつ、確実に。がんばるぞいっ！(๑•̀ㅂ•́)و✧**

</div>
