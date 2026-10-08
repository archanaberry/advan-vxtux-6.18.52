# VXTux 💜✨

<div align="center">

[VXTux-chan](archanaberry/vxtux_chan.png)

### Linux 6.18.52 for Advan Tab VX Lite (T1030) desu~ (≧▽≦)/

`ARM64` / `Unisoc UMS512-T618` / `VXTux`

**一歩ずつ、確実に。 Device-first kernel work, documented with care. (^_^)/**

*Halo nyaa~ Hello nyaa~ こんにちはなの~ 💕*

</div>

---

## 🌸 Tentang proyek | About | プロジェクトについてなの

> **ID (Indonesia) — Imut version:** Halo halo~ (｡•́︿•̀｡) VXTux ini adalah kernel Linux yang fokus buat device kesayangan kita **Advan Tab VX Lite T1030** nyaa~ Platform targetnya Unisoc UMS512 yang juga dipanggil T618 desu! Kerjaannya gabungin driver upstream Linux yang baik hati sama port platform yang di-scope hati-hati dan support device-tree~ Bukan sekadar config dump loh! (｀・ω・´) Semua pilihan hardware itu nyambung sama DTS target, dependensi Kconfig, sama wiring build kalau source tree ngasih bukti nyaa~ ✨

> **EN (English) — Cute version:** Hewwooo~ (≧∇≦)/ VXTux is a super device-focused Linux kernel tree for our beloved **Advan Tab VX Lite T1030** nano~ The target platform is Unisoc UMS512, also known as T618 desu! The work combines kind upstream Linux drivers with carefully scoped platform ports and device-tree support nya~ Not just a config dump, okay? (｡>﹏<｡) Hardware-specific choices are tied to the target DTS, Kconfig dependencies, and build wiring wherever the source tree provides that evidence, desu!

> **JP (日本語) — 萌えバージョン:** こんにちはなの〜 (⁄˶´-`⁄)♡ VXTuxは、大好きな **Advan Tab VX Lite T1030** のためのデバイスファーストなLinuxカーネルツリーなの！ターゲットはUnisoc UMS512、別名T618なのです〜。 upstreamの優しいドライバーと、丁寧にスコープされたプラットフォームポート、そしてデバイスツリーサポートを組み合わせてるの！ただのconfig dumpじゃないんだからねっ！(๑•̀ㅂ•́)و✧ ハードウェア固有の選択は、ターゲットDTSやKconfig依存関係、ビルドの配線に紐づけて、ソースツリーが証拠をくれる限りちゃんと記録してるの、えへへ〜

---

## 🎯 Target | ターゲットだよっ

| Area | Target desu~ |
| --- | --- |
| Device | Advan Tab VX Lite T1030 📱 |
| SoC | Unisoc UMS512 / T618 |
| Architecture | ARM64 |
| Kernel | Linux 6.18.52-VXTux-UMS512 |
| Device tree | `ums512-1h10-vxtux.dtb` 🌟 |
| Graphics | Mali-G52 MP2 (Bifrost, vendor codename Gondul) via Panfrost desu! |

---

## 💜 Driver tally | ドライバー集計なのっ | Hitungan driver

<div align="center">

| Count | Scope nyaa~ |
| ---: | --- |
| 102 | Deblob-derived driver integrations in the project tally (｡•̀ᴗ-)✧ |
| 1 | Panfrost, the open-source kernel GPU driver replacing the vendor Mali Gondul DDK path nyaa~ |
| **103** | **Tracked driver integrations: 102 + 1 desu! (≧▽≦)** |

</div>

> **ID:** Panfrost itu dihitung terpisah dari 102 entri deblob yaa~ Ini hitungan integrasi driver, **bukan** hitungan model GPU (´｡• ω •｡`) Snapshot source sekarang belum include ledger deblob yang terperinci, jadi angka 102 itu ngikutin project tally bukan list per-driver yang reproducible di repo ini, gomen ne~ (｡•́︿•̀｡)

> **EN:** Panfrost is counted separately from the 102 deblob-derived entries, okay? This is a driver-integration count, **not** a count of GPU models, nano! The current source snapshot does not include an itemized deblob ledger, so the 102 figure reflects the project tally rather than a reproducible per-driver list in this repository, gomen~ (´｡• ᵕ •｡`)

> **JP:** Panfrostは102のdeblob由来エントリとは別カウントなの！これはドライバー統合数のカウントで、**GPUモデル数のカウントじゃない**んだからねっ！現在のソーススナップショットは詳細なdeblob台帳を含んでいないので、102という数字はこのリポジトリで再現可能なドライバーごとのリストというより、プロジェクトの集計を反映したものなの、ごめんね〜 (｡>﹏<｡)

---

### 📋 Inventory per entri | Driver inventory | ドライバー在庫なの

<details>
<summary><b>Click to expand 102 identifiers desu~ (｡•̀ᴗ-)✧ / クリックしてねっ</b></summary>

> Ledger order-nya dipertahanin biar auditable yaa~ Source links point into current kernel tree. Kalau identifier itu composite module, alias, header-only, atau donor name tanpa file standalone yang matching, yang di-link itu Kbuild file atau nearest source dan statusnya bilang gitu, desu! Duplicate source paths itu intentional ledger records, bukan additional unique implementations, okay? (｀・ω・´)

| # | Ledger ID | Source in this tree | Status / evidence nyaa~ |
| ---: | --- | --- | --- |
| 1 | `aes-ce-ccm` | [arch/arm64/crypto/aes-ce-ccm-glue.c](6.18.52-VXTux-UMS512/arch/arm64/crypto/aes-ce-ccm-glue.c) | Mainline implementation plus ARM64 glue desu! |
| 2 | `aes-neon-blk` | [arch/arm64/crypto/aes-glue-neon.c](6.18.52-VXTux-UMS512/arch/arm64/crypto/aes-glue-neon.c) | Mainline; module form in ledger nyaa~ |
| 3 | `agsd` | [drivers/sound/soc/sprd/agdsp_access/agdsp_access.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/agdsp_access/agdsp_access.c) | Re-mapped; Tier A/B evidence (｡•̀ᴗ-)✧ |
| 4 | `apsys-dvfs` | [drivers/devfreq/sprd/apsys/sprd_dvfs_apsys.c](6.18.52-VXTux-UMS512/drivers/devfreq/sprd/apsys/sprd_dvfs_apsys.c) | SPRD devfreq port desu! |
| 5 | `arc4` | [crypto/arc4.c](6.18.52-VXTux-UMS512/crypto/arc4.c) | Mainline; ADA-verified in ledger |
| 6 | `asix` | [drivers/net/usb/asix_common.c](6.18.52-VXTux-UMS512/drivers/net/usb/asix_common.c) | Mainline USB Ethernet; module nyaa~ |
| 7 | `audio-dsp-dump` | [drivers/sound/soc/sprd/audiodspdump/audio_dsp_dump.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audiodspdump/audio_dsp_dump.c) | SPRD port; module |
| 8 | `audio-pipe` | [drivers/sound/soc/sprd/audiosipc/audio-pipe.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audiosipc/audio-pipe.c) | UNPROVEN; requires runtime/System.map evidence, gomen~ (｡•́︿•̀｡) |
| 9 | `audio_mem` | [drivers/sound/soc/sprd/audiomem/audio_mem.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audiomem/audio_mem.c) | Path moved from donor layout; GPL fork source |
| 10 | `audio_sipc` | [drivers/sound/soc/sprd/audiosipc/audio-sipc.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audiosipc/audio-sipc.c) | GPL fork port desu! |
| 11 | `ax88179_178a` | [drivers/net/usb/ax88179_178a.c](6.18.52-VXTux-UMS512/drivers/net/usb/ax88179_178a.c) | Mainline USB Ethernet; module |
| 12 | `bq2560x-charger` | [drivers/power/supply/bq256xx_charger.c](6.18.52-VXTux-UMS512/drivers/power/supply/bq256xx_charger.c) | UNPROVEN; compatible alias is separately patched |
| 13 | `core` | [drivers/net/wireless/sprd/sprdwcn/platform/Makefile](6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/platform/Makefile) | Composite `wcn_core` module; no standalone `core.c` |
| 14 | `cpufreq_userspace` | [drivers/cpufreq/cpufreq_userspace.c](6.18.52-VXTux-UMS512/drivers/cpufreq/cpufreq_userspace.c) | Mainline |
| 15 | `extcon-usb-gpio` | [drivers/extcon/extcon-usb-gpio.c](6.18.52-VXTux-UMS512/drivers/extcon/extcon-usb-gpio.c) | Mainline |
| 16 | `ghash-ce` | [arch/arm64/crypto/ghash-ce-glue.c](6.18.52-VXTux-UMS512/arch/arm64/crypto/ghash-ce-glue.c) | Mainline implementation plus ARM64 glue |
| 17 | `gnss_common_ctl_all` | [drivers/net/wireless/sprd/sprdwcn/platform/gnss/gnss_common_ctl.c](6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/platform/gnss/gnss_common_ctl.c) | Member of composite `wcn_core` |
| 18 | `gnss_dbg` | [drivers/net/wireless/sprd/sprdwcn/platform/gnss/gnss_dbg.c](6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/platform/gnss/gnss_dbg.c) | Member of composite `wcn_core` |
| 19 | `gnss_pmnotify_ctl` | [drivers/net/wireless/sprd/sprdwcn/platform/gnss/gnss_pmnotify_ctl.c](6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/platform/gnss/gnss_pmnotify_ctl.c) | Member of composite `wcn_core` |
| 20 | `gpio` | [drivers/gpio/gpio-sprd.c](6.18.52-VXTux-UMS512/drivers/gpio/gpio-sprd.c) | Tier A desu! (≧▽≦) |
| 21 | `hxchipset_83102e` | [drivers/input/touchscreen/sprd/hx83102e/hxchipset/himax_platform.c](6.18.52-VXTux-UMS512/drivers/input/touchscreen/sprd/hx83102e/hxchipset/himax_platform.c) | Himax public-tree source |
| 22 | `ims_bridge` | [net/ims_bridge/Makefile](6.18.52-VXTux-UMS512/net/ims_bridge/Makefile) | Tier A; composite network module |
| 23 | `ion_cma_heap` | [drivers/dma-buf/heaps/cma_heap.c](6.18.52-VXTux-UMS512/drivers/dma-buf/heaps/cma_heap.c) | UNPROVEN as a deblob replacement |
| 24 | `ion_ipc_trusty` | [drivers/dma-buf/sprd/ion_ipc_trusty.c](6.18.52-VXTux-UMS512/drivers/dma-buf/sprd/ion_ipc_trusty.c) | Noble substitute; ported 2026-10-07 nyaa~ (｡•̀ᴗ-)✧ |
| 25 | `jpg` | [drivers/misc/sprd_jpg/sprd_jpg.c](6.18.52-VXTux-UMS512/drivers/misc/sprd_jpg/sprd_jpg.c) | SPRD JPEG port |
| 26 | `leds-sc27xx-bltc` | [drivers/leds/leds-sc27xx-bltc.c](6.18.52-VXTux-UMS512/drivers/leds/leds-sc27xx-bltc.c) | UNPROVEN |
| 27 | `ledtrig-pattern` | [drivers/leds/trigger/ledtrig-pattern.c](6.18.52-VXTux-UMS512/drivers/leds/trigger/ledtrig-pattern.c) | UNPROVEN |
| 28 | `mcdt_hw` | [drivers/sound/soc/sprd/mcdt/mcdt_r1p0/mcdt_hw.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/mcdt/mcdt_r1p0/mcdt_hw.c) | Resolved through `sprd_mcdt_*` symbols |
| 29 | `mipi_driver` | Not present under the supplied `drivers/unisoc_platform/debug_log/sharkl5/` path | Noble substitute listed; source path absent from this snapshot, gomen~ |
| 30 | `misc_sprd_uid` | [drivers/soc/sprd/soc_id/sprd_uid.c](6.18.52-VXTux-UMS512/drivers/soc/sprd/soc_id/sprd_uid.c) | Tier A |
| 31 | `musb_hdrc` | [drivers/usb/musb/musb_core.c](6.18.52-VXTux-UMS512/drivers/usb/musb/musb_core.c) | Noble substitute; linkage remains unverified |
| 32 | `musb_sprd` | [drivers/usb/musb/musb_sprd_vxtux.c](6.18.52-VXTux-UMS512/drivers/usb/musb/musb_sprd_vxtux.c) | Noble substitute; partial validation |
| 33 | `novatek_ts_spi` | [drivers/input/touchscreen/novatek-nvt-ts-spi.c](6.18.52-VXTux-UMS512/drivers/input/touchscreen/novatek-nvt-ts-spi.c) | Noble substitute; not claimed as a one-to-one replacement |
| 34 | `phy-sprd-sharkl5Pro` | [drivers/phy/sprd/phy-sprd-ums512.c](6.18.52-VXTux-UMS512/drivers/phy/sprd/phy-sprd-ums512.c) | Tier A |
| 35 | `pinctrl` | [drivers/pinctrl/sprd/pinctrl-sprd.c](6.18.52-VXTux-UMS512/drivers/pinctrl/sprd/pinctrl-sprd.c) | Tier A; shared source with next ledger entry |
| 36 | `pinctrl-sprd` | [drivers/pinctrl/sprd/pinctrl-sprd.c](6.18.52-VXTux-UMS512/drivers/pinctrl/sprd/pinctrl-sprd.c) | Tier A; same source path as previous entry |
| 37 | `pinctrl-sprd-sharkl5Pro` | [drivers/pinctrl/sprd/pinctrl-sprd-sharkl5pro.c](6.18.52-VXTux-UMS512/drivers/pinctrl/sprd/pinctrl-sprd-sharkl5pro.c) | Tier A |
| 38 | `pwm-sprd` | [drivers/pwm/pwm-sprd.c](6.18.52-VXTux-UMS512/drivers/pwm/pwm-sprd.c) | Tier A |
| 39 | `pwm_bl` | [drivers/video/backlight/pwm_bl.c](6.18.52-VXTux-UMS512/drivers/video/backlight/pwm_bl.c) | Mainline PWM backlight |
| 40 | `rtc-sc27xx` | [drivers/rtc/rtc-sc27xx.c](6.18.52-VXTux-UMS512/drivers/rtc/rtc-sc27xx.c) | Tier A |
| 41 | `sc27xx-poweroff` | [drivers/power/reset/sc27xx-poweroff.c](6.18.52-VXTux-UMS512/drivers/power/reset/sc27xx-poweroff.c) | Tier A |
| 42 | `sc27xx-vibra` | [drivers/input/misc/sc27xx-vibra.c](6.18.52-VXTux-UMS512/drivers/input/misc/sc27xx-vibra.c) | UNPROVEN |
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
| 54 | `snd-soc-sprd-vbc-fe` | [drivers/sound/soc/sprd/dai_v4/vbc-dai.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/dai_v4/vbc-dai.c) | UNPROVEN |
| 55 | `snd-soc-sprd-vbc-v4` | [drivers/sound/soc/sprd/dai_v4/vbc-phy-v4.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/dai_v4/vbc-phy-v4.c) | UNPROVEN |
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
| 71 | `sprd_camera` | [drivers/media/platform/sprd/pipeline/sprd_dcam.c](6.18.52-VXTux-UMS512/drivers/media/platform/sprd/pipeline/sprd_dcam.c) | UNPROVEN; ledger correction dated 2026-10-05 |
| 72 | `sprd_camsys_pw_domain` | [drivers/pmdomain/sprd/sprd_camsys_pw_domain.c](6.18.52-VXTux-UMS512/drivers/pmdomain/sprd/sprd_camsys_pw_domain.c) | Power-domain port |
| 73 | `sprd_cp_dvfs` | [drivers/devfreq/sprd/apsys/sprd_dvfs_apsys.c](6.18.52-VXTux-UMS512/drivers/devfreq/sprd/apsys/sprd_dvfs_apsys.c) | Same object as `apsys-dvfs` |
| 74 | `sprd_cpp` | [drivers/media/platform/sprd/pipeline/sprd_isp.c](6.18.52-VXTux-UMS512/drivers/media/platform/sprd/pipeline/sprd_isp.c) | UNPROVEN; ledger correction dated 2026-05 |
| 75 | `sprd_cpu_cooling` | [drivers/thermal/sprd/sprd_cpu_cooling.c](6.18.52-VXTux-UMS512/drivers/thermal/sprd/sprd_cpu_cooling.c) | Tier A |
| 76 | `sprd_ddr_dvfs` | [drivers/devfreq/sprd/apsys/sprd_dvfs_apsys.c](6.18.52-VXTux-UMS512/drivers/devfreq/sprd/apsys/sprd_dvfs_apsys.c) | Same object as `apsys-dvfs` |
| 77 | `sprd_flash_drv` | [drivers/sprd/flash/flash_drv.c](6.18.52-VXTux-UMS512/drivers/sprd/flash/flash_drv.c) | Tier A |
| 78 | `sprd_fm` | [drivers/net/wireless/sprd/sprdwcn/fm/driver/fm_sdio/fmdrv_main.c](6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/fm/driver/fm_sdio/fmdrv_main.c) | Tier A; 4/4 objects, zero compile errors per ledger |
| 79 | `sprd_gpu_cooling` | [drivers/thermal/sprd/sprd_gpu_cooling.c](6.18.52-VXTux-UMS512/drivers/thermal/sprd/sprd_gpu_cooling.c) | Tier A |
| 80 | `sprd_map` | [drivers/misc/sprd/sprd_map.c](6.18.52-VXTux-UMS512/drivers/misc/sprd/sprd_map.c) | Tier A |
| 81 | `sprd_mipi` | Not present under the supplied `drivers/unisoc_platform/debug_log/` path | UNPROVEN; no matching source path in this snapshot |
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
| 94 | `trusty-tui` | [drivers/misc/trusty-tui.c](6.18.52-VXTux-UMS512/drivers/misc/trusty-tui.c) | Port of vendor `trusty-tui.ko` nyaa~ |
| 95 | `twofish_common` | [crypto/twofish_common.c](6.18.52-VXTux-UMS512/crypto/twofish_common.c) | Mainline |
| 96 | `twofish_generic` | [crypto/twofish_generic.c](6.18.52-VXTux-UMS512/crypto/twofish_generic.c) | Mainline |
| 97 | `unisoc-iommu` | [drivers/iommu/sprd-iommu.c](6.18.52-VXTux-UMS512/drivers/iommu/sprd-iommu.c) | Tier A |
| 98 | `virt-dma` | [drivers/dma/virt-dma.c](6.18.52-VXTux-UMS512/drivers/dma/virt-dma.c) | Tier A shared DMA helper |
| 99 | `vsp` | [drivers/pmdomain/sprd/vsp_regs.c](6.18.52-VXTux-UMS512/drivers/pmdomain/sprd/vsp_regs.c) | VSP domain support; see adjacent VSP headers and gate driver |
| 100 | `wcn_bsp` | [drivers/net/wireless/sprd/sprdwcn/platform/Makefile](6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/platform/Makefile) | WCN platform module composition; no standalone `bsp.c` found |
| 101 | `zram` | [drivers/block/zram/zram_drv.c](6.18.52-VXTux-UMS512/drivers/block/zram/zram_drv.c) | N/A/absence entry in the supplied tally; corrected 2026-10-05 |
| 102 | `zsmalloc` | [mm/zsmalloc.c](6.18.52-VXTux-UMS512/mm/zsmalloc.c) | UNPROVEN in the supplied tally; corrected 2026-10-05 |
| 103 | `panfrost` | [drivers/gpu/drm/panfrost/panfrost_gpu.c](6.18.52-VXTux-UMS512/drivers/gpu/drm/panfrost/panfrost_gpu.c) | Separate open-source GPU driver; Mali-G52 MP2 / Gondul replacement desu! ✨ |

</details>

> **Count desu:** 102 supplied deblob/port ledger identifiers + 1 Panfrost entry = **103 documented entries** nyaa~ (≧▽≦) This number intentionally preserves aliases and shared source paths from the ledger; it is not a claim of 103 distinct, independently validated kernel modules. `UNPROVEN`, `NOT-CONFIGURED`, `N/A`, and source-not-present labels are retained so the tally stays auditable, okay? (｡•̀ᴗ-)✧

---

## 🎮 Graphics note | GPU メモなのっ

> **ID:** GPU T618 itu Mali-G52 MP2, bukan Mali-G57 yaa~ (´｡• ω •｡`) Stock GPU ID `0x7402` itu dinormalisasi sama Panfrost model comparison jadi `0x7002`, matching G52 model entry nya desu! Target DTS pake binding Panfrost `arm,mali-bifrost`; dia gak expose removed vendor DDK compatible lagi kok!

> **EN:** The T618 GPU is Mali-G52 MP2, not Mali-G57, okay? (｡>﹏<｡) The stock GPU ID `0x7402` is normalized by Panfrost's model comparison to `0x7002`, matching its G52 model entry, nano! The target DTS uses the Panfrost binding `arm,mali-bifrost`; it does not expose the removed vendor DDK compatible.

> **JP:** T618のGPUはMali-G57じゃなくてMali-G52 MP2なの！ストックのGPU ID `0x7402`はPanfrostのモデル比較で`0x7002`に正規化されて、G52のモデルエントリーにマッチするの〜！ターゲットDTSはPanfrostバインディングの`arm,mali-bifrost`を使ってて、削除されたベンダーDDKのcompatibleはもう公開してないの、えへへ〜

> Panfrost replaces the **kernel-side GPU driver** desu~ Userspace graphics libraries are a separate layer and are not implied by this driver count, okay? (｡•̀ᴗ-)✧

---

## 🛠️ Build | ビルド方法なのっ | Cara build

> **ID:** Pake clean kernel source tree yaa~ Kbuild nolak out-of-tree builds kalau ada `.config` di source-root (｡•́︿•̀｡) Install dulu build dependencies yang biasa, termasuk `flex`, `bison`, sama AArch64 toolchain nyaa~

> **EN:** Use a clean kernel source tree, nano~ Kbuild rejects out-of-tree builds when a source-root `.config` is present (´｡• ᵕ •｡`) Install the usual kernel build dependencies, including `flex`, `bison`, and an AArch64 toolchain, desu!

> **JP:** クリーンなカーネルソースツリーを使ってね〜 ソースルートに`.config`があるとKbuildがout-of-treeビルドを拒否しちゃうの (｡•́︿•̀｡) `flex`や`bison`、AArch64ツールチェーンを含む、いつものカーネルビルド依存関係をインストールしてねっ！

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

> Target DTB nya bakal ada di sini desu~ (≧▽≦)/

```text
out/arch/arm64/boot/dts/sprd/ums512-1h10-vxtux.dtb
```

---

## 📝 Configuration notes | メモだよっ

- **ID:** Production fragment itu targetnya UMS512/T618 yaa~ Seleksi SoC Qualcomm sama MediaTek dimatiin buat target ini; source driver upstreamnya gak dihapus kok, cuma dimatiin (｡•̀ᴗ-)✧

  **EN:** The production fragment targets UMS512/T618, nano~ Qualcomm and MediaTek SoC selections are disabled for this target; their upstream driver sources are not deleted.

  **JP:** プロダクションフラグメントはUMS512/T618をターゲットにしてるの〜 このターゲットではQualcommとMediaTekのSoC選択は無効化されてるけど、upstreamのドライバーソース自体は削除されてないの！

- **ID:** Support Generic PCI/PCIe, USB, mass-storage, eMMC, sama UFS tetep ada kok! Board DTS sama hardware wiring yang sekarang yang nentuin device mana yang nge-bind pas runtime, desu!

  **EN:** Generic PCI/PCIe, USB, mass-storage, eMMC, and UFS support remain available, nano~ The board DTS and current hardware wiring determine which devices bind at runtime.

  **JP:** 汎用PCI/PCIe、USB、マスストレージ、eMMC、UFSサポートはまだあるの！ボードDTSと現在のハードウェア配線が、ランタイム時にどのデバイスがバインドされるかを決めるの〜

- **ID:** Board UMS512 pake Unisoc USB path sama SDIO-based WCN configuration nyaa~ Opsi yang gak kepake gak usah dijadiin alasan buat ngapus reusable driver source yaa! (｀・ω・´)

  **EN:** The UMS512 board uses its Unisoc USB path and SDIO-based WCN configuration. Unused platform-specific options do not justify removing reusable driver source, okay?

  **JP:** UMS512ボードはUnisocのUSBパスとSDIOベースのWCN設定を使ってるの！使われてないプラットフォーム固有のオプションは、再利用可能なドライバーソースを削除する理由にはならないんだからねっ！

- **ID:** Firmware sama device-tree sources tetep ada di kernel tree kok~ Availability firmware sama kebutuhan userspace itu urusan device-image dan harus dicek terpisah yaa~ (｡•́︿•̀｡)

  **EN:** Firmware and device-tree sources remain in the kernel tree. Firmware availability and userspace requirements are device-image concerns and should be checked separately, desu!

  **JP:** ファームウェアとデバイスツリーソースはカーネルツリーに残ってるの！ファームウェアの可用性とユーザースペースの要件はデバイスイメージ側の関心事で、別途チェックしてねっ！

---

## 🔧 Status | 状態なのっ | Status desu~

> **ID:** Tree ini lagi active bring-up nih~ (≧∇≦) Config selection atau DTB build yang sukses aja belum ngebuktiin kalau driver udah divalidasi di hardware fisik yaa~ Runtime support harus dikonfirmasi pake probe logs sama device tests, okay? (｡•̀ᴗ-)✧

> **EN:** This tree is under active bring-up, nano~ A config selection or successful DTB build alone does not prove that a driver has been validated on physical hardware; runtime support should be confirmed with probe logs and device tests, desu! (´｡• ᵕ •｡`)

> **JP:** このツリーは現在アクティブにbring-up中なの〜 config選択やDTBビルドが成功しただけでは、ドライバーが物理ハードウェアで検証されたことの証明にはならないの！ランタイムサポートはprobeログとデバイステストで確認してねっ！(｡>﹏<｡)

Issue reports sama patches harus include exact board revision, kernel config, `dmesg` output yang relevan, sama reproduction steps yaa~ どうぞよろしくお願いしますなのっ！(≧▽≦)/💕

---

<div align="center">

**VXTux-chan says: 一歩ずつ、確実に。がんばるぞいっ！(๑•̀ㅂ•́)و✧**

*Made with 💜 by archanaberry for Advan Tab VX Lite comrades desu~*

</div>
