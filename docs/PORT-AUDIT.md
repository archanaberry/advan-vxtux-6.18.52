# PORT AUDIT — 129 .ko vendor Advan vs tree 6.18.52-VXTux-UMS512

Dibuat otomatis oleh `donors/port_audit.py` (jangan edit manual).

Status: **KO** = ter-build .ko · **Y** = sumber ada + `=y` (built-in) · **OBJ-Y** = selalu built-in · **OFF** = sumber + Kconfig ada, belum diaktifkan · **NOKCONFIG** = Makefile pakai CONFIG_ tapi entri Kconfig hilang · **OFF-DESIGN** = sengaja `# ... is not set` · **NOSRC** = sumber driver tidak ada di tree (butuh port kode baru)

## Ringkasan

| Status | Jumlah |
|---|---:|
| KO | 71 |
| Y | 57 |
| OFF | 1 |
| NOSRC | 24 |
| **total** | **153** |

## Tabel

| # | Driver | Status | Simbol | Sumber |
|---:|---|---|---|---|
| 1 | `aes-ce-ccm.ko` | KO | `CONFIG_CRYPTO_AES_ARM64_CE_CCM` | `./arch/arm64/crypto` |
| 2 | `aes-neon-blk.ko` | KO | `CONFIG_CRYPTO_AES_ARM64_NEON_BLK` | `./arch/arm64/crypto` |
| 3 | `aes-neon-blk.ko` | KO | `CONFIG_CRYPTO_AES_ARM64_NEON_BLK` | `./arch/arm64/crypto` |
| 4 | `agdsp_access.ko` | KO | `CONFIG_SPRD_AUDIO_AGDSP_ACCESS` | `./drivers/vxtux/audio/agdsp_access` |
| 5 | `apsys-dvfs.ko` | KO | `CONFIG_SPRD_APSYS_DVFS` | `./drivers/devfreq/sprd/apsys` |
| 6 | `asix.ko` | KO | `CONFIG_USB_NET_AX8817X` | `./drivers/net/usb` |
| 7 | `asix.ko` | KO | `CONFIG_USB_NET_AX8817X` | `./drivers/net/usb` |
| 8 | `audio-dsp-dump.ko` | KO | `CONFIG_SPRD_AUDIO_DSP_DUMP` | `./drivers/vxtux/audio/audiodspdump` |
| 9 | `audio-dsp-dump.ko` | KO | `CONFIG_SPRD_AUDIO_DSP_DUMP` | `./drivers/vxtux/audio/audiodspdump` |
| 10 | `audio-pipe.ko` | KO | `CONFIG_SPRD_AUDIO_PIPE` | `./drivers/vxtux/audio/audiosipc` |
| 11 | `audio_sipc.ko` | KO | `CONFIG_SPRD_AUDIO_SIPC` | `./drivers/vxtux/audio/audiosipc` |
| 12 | `ax88179_178a.ko` | KO | `CONFIG_USB_NET_AX88179_178A` | `./drivers/net/usb` |
| 13 | `ax88179_178a.ko` | KO | `CONFIG_USB_NET_AX88179_178A` | `./drivers/net/usb` |
| 14 | `clk-sprd.ko` | KO | `CONFIG_SPRD_COMMON_CLK` | `./drivers/clk/sprd` |
| 15 | `extcon-usb-gpio.ko` | KO | `CONFIG_EXTCON_USB_GPIO` | `./drivers/extcon` |
| 16 | `gpio-pmic-eic-sprd.ko` | KO | `CONFIG_GPIO_PMIC_EIC_SPRD` | `./drivers/gpio` |
| 17 | `ion_ipc_trusty.ko` | KO | `CONFIG_ION_IPC_TRUSTY` | `./drivers/dma-buf/sprd` |
| 18 | `mipi_driver.ko` | KO | `CONFIG_SPRD_MIPI_LOG_R5P0` | `./drivers/unisoc_platform/debug_log/sharkl5` |
| 19 | `mmc_hsq.ko` | KO | `CONFIG_MMC_HSQ` | `./drivers/mmc/host` |
| 20 | `mmdvfs.ko` | KO | `CONFIG_SPRD_MM_DVFS` | `./drivers/devfreq/sprd/mm` |
| 21 | `novatek_ts_spi.ko` | KO | `CONFIG_TOUCHSCREEN_NOVATEK_NVT_TS_SPI` | `./drivers/input/touchscreen/novatek_nvt_ts_spi` |
| 22 | `novatek_ts_spi.ko` | KO | `CONFIG_TOUCHSCREEN_NOVATEK_NVT_TS_SPI` | `./drivers/input/touchscreen/novatek_nvt_ts_spi` |
| 23 | `nvmem-sc27xx-efuse.ko` | KO | `CONFIG_NVMEM_SC27XX_EFUSE` | `./drivers/nvmem` |
| 24 | `nvmem_sprd_cache_efuse.ko` | KO | `CONFIG_NVMEM_SPRD_CACHE_EFUSE` | `./drivers/vxtux/soc/cache_efuse` |
| 25 | `nvmem_sprd_efuse.ko` | KO | `CONFIG_NVMEM_SPRD_EFUSE` | `./drivers/nvmem` |
| 26 | `phy-sprd-sharkl5Pro.ko` | KO | `CONFIG_PHY_SPRD_UMS512_USB2` | `./drivers/phy/sprd` |
| 27 | `pwm_bl.ko` | KO | `CONFIG_BACKLIGHT_PWM` | `./drivers/video/backlight` |
| 28 | `pwm_bl.ko` | KO | `CONFIG_BACKLIGHT_PWM` | `./drivers/video/backlight` |
| 29 | `rtc-sc27xx.ko` | KO | `CONFIG_RTC_DRV_SC27XX` | `./drivers/rtc` |
| 30 | `sc2730-regulator.ko` | KO | `CONFIG_REGULATOR_SC2730` | `./drivers/regulator` |
| 31 | `sc27xx_adc.ko` | KO | `CONFIG_SC27XX_ADC` | `./drivers/iio/adc` |
| 32 | `sc27xx_typec.ko` | KO | `CONFIG_TYPEC_SPRD` | `./drivers/usb/typec/sprd` |
| 33 | `sdhci-sprd.ko` | KO | `CONFIG_MMC_SDHCI_SPRD` | `./drivers/mmc/host` |
| 34 | `sensorhub.ko` | KO | `CONFIG_IIO_SSP_SENSORHUB` | `./drivers/iio/common/ssp_sensors` |
| 35 | `slog_bridge.ko` | KO | `CONFIG_SPRD_SIPC_SLOG_BRIDGE` | `./drivers/vxtux/sipc` |
| 36 | `snd-soc-sprd-vbc-fe.ko` | KO | `CONFIG_VXTUX_SND_SPRD_VBC_V4` | `./drivers/vxtux/sound/asoc/dai_v4` |
| 37 | `snd-soc-sprd-vbc-v4.ko` | KO | `CONFIG_VXTUX_SND_SPRD_VBC_V4` | `./drivers/vxtux/sound/asoc/dai_v4` |
| 38 | `spi-sprd.ko` | KO | `CONFIG_SPI_SPRD` | `./drivers/spi` |
| 39 | `sprd-dmaengine-pcm.ko` | KO | `CONFIG_VXTUX_SND_SPRD_DMAENGINE_PCM` | `./drivers/vxtux/sound/asoc/platform` |
| 40 | `sprd-drm.ko` | KO | `CONFIG_DRM_SPRD` | `./drivers/gpu/drm/sprd` |
| 41 | `sprd-drm.ko` | KO | `CONFIG_DRM_SPRD` | `./drivers/gpu/drm/sprd` |
| 42 | `sprd-gsp.ko` | KO | `CONFIG_DRM_SPRD_GSP` | `./drivers/gpu/drm/sprd/gsp` |
| 43 | `sprd-gsp.ko` | KO | `CONFIG_DRM_SPRD_GSP` | `./drivers/gpu/drm/sprd/gsp` |
| 44 | `sprd-sipc-virt-bus.ko` | KO | `CONFIG_SPRD_SIPC_VIRT_BUS` | `./drivers/vxtux/sipc` |
| 45 | `sprd_apipe.ko` | KO | `CONFIG_SPRD_APIPE` | `./drivers/vxtux/audio/audio_pipe` |
| 46 | `sprd_audcp_boot.ko` | KO | `CONFIG_SPRD_AUDIO_CPBOOT` | `./drivers/vxtux/audio/audiocpboot` |
| 47 | `sprd_audcp_dvfs.ko` | KO | `CONFIG_SPRD_HW_DEVICE_DVFS_AGCP` | `./drivers/vxtux/audio/audiodvfs` |
| 48 | `sprd_battery_info.ko` | KO | `CONFIG_SPRD_BATTERY_INFO` | `./drivers/power/supply/sprd` |
| 49 | `sprd_battery_info.ko` | KO | `CONFIG_SPRD_BATTERY_INFO` | `./drivers/power/supply/sprd` |
| 50 | `sprd_battery_info.ko` | KO | `CONFIG_SPRD_BATTERY_INFO` | `./drivers/power/supply/sprd` |
| 51 | `sprd_flash_drv.ko` | KO | `CONFIG_SPRD_FLASH_DRV` | `./drivers/sprd/flash` |
| 52 | `sprd_mipi.ko` | KO | `CONFIG_SPRD_MODEM_DEBUG` | `./drivers/unisoc_platform/debug_log` |
| 53 | `sprd_power_manager.ko` | KO | `CONFIG_SPRD_POWER_MANAGER` | `./drivers/soc/sprd/power_manager` |
| 54 | `sprd_power_manager.ko` | KO | `CONFIG_SPRD_POWER_MANAGER` | `./drivers/soc/sprd/power_manager` |
| 55 | `sprd_shm.ko` | KO | `CONFIG_SPRD_TSHM` | `./drivers/trusty` |
| 56 | `sprd_systimer.ko` | KO | `CONFIG_SPRD_SYSTIMER` | `./drivers/clocksource` |
| 57 | `sprd_time_sync.ko` | KO | `CONFIG_SPRD_TIME_SYNC` | `./drivers/soc/sprd/time_sync` |
| 58 | `sprd_time_sync_cp.ko` | KO | `CONFIG_SPRD_TIME_SYNC_CP` | `./drivers/soc/sprd/time_sync` |
| 59 | `sprdwl.ko` | KO | `CONFIG_WCN_WLAN` | `./drivers/net/wireless/sprd/sprdwcn/wlan` |
| 60 | `trusty-ipc.ko` | KO | `CONFIG_TRUSTY_VIRTIO_IPC` | `./drivers/trusty` |
| 61 | `trusty-irq.ko` | KO | `CONFIG_TRUSTY` | `./drivers/trusty` |
| 62 | `trusty-log.ko` | KO | `CONFIG_TRUSTY_LOG` | `./drivers/trusty` |
| 63 | `trusty-log.ko` | KO | `CONFIG_TRUSTY_LOG` | `./drivers/trusty` |
| 64 | `trusty-pm.ko` | KO | `CONFIG_TRUSTY` | `./drivers/trusty` |
| 65 | `trusty-tui.ko` | KO | `CONFIG_TRUSTY_TUI` | `./drivers/misc` |
| 66 | `trusty-virtio.ko` | KO | `CONFIG_TRUSTY_VIRTIO` | `./drivers/trusty` |
| 67 | `trusty.ko` | KO | `CONFIG_TRUSTY` | `./drivers/trusty` |
| 68 | `ums512-clk.ko` | KO | `CONFIG_SPRD_UMS512_CLK` | `./drivers/clk/sprd` |
| 69 | `unisoc-mailbox.ko` | KO | `CONFIG_SPRD_MBOX` | `./drivers/mailbox` |
| 70 | `usb_f_vser.ko` | KO | `CONFIG_USB_F_VSERIAL` | `./drivers/usb/gadget/function` |
| 71 | `wcn_core.ko` | KO | `CONFIG_SPRD_LEGACY_WCN_CORE` | `./drivers/net/wireless/sprd/sprdwcn/platform` |
| 72 | `arc4.ko` | Y | `CONFIG_CRYPTO_ARC4` | `./crypto` |
| 73 | `audio_mem.ko` | Y | `CONFIG_SPRD_AUDIO_MEMORY_MANAGER` | `./drivers/vxtux/audio/audiomem` |
| 74 | `bq2560x-charger.ko` | Y | `CONFIG_CHARGER_BQ256XX` | `./drivers/power/supply` |
| 75 | `core.ko` | Y | `CONFIG_KPROBES` | `./arch/arm/probes/kprobes` |
| 76 | `ghash-ce.ko` | Y | `CONFIG_CRYPTO_GHASH_ARM64_CE` | `./arch/arm64/crypto` |
| 77 | `gpio-eic-sprd.ko` | Y | `CONFIG_GPIO_EIC_SPRD` | `./drivers/gpio` |
| 78 | `gpio-sprd.ko` | Y | `CONFIG_GPIO_SPRD` | `./drivers/gpio` |
| 79 | `i2c-sprd.ko` | Y | `CONFIG_I2C_SPRD` | `./drivers/i2c/busses` |
| 80 | `jpg.ko` | Y | `CONFIG_SPRD_JPG` | `./drivers/misc/sprd_jpg` |
| 81 | `kfifo_buf.ko` | Y | `CONFIG_IIO_KFIFO_BUF` | `./drivers/iio/buffer` |
| 82 | `kfifo_buf.ko` | Y | `CONFIG_IIO_KFIFO_BUF` | `./drivers/iio/buffer` |
| 83 | `leds-sc27xx-bltc.ko` | Y | `CONFIG_LEDS_SC27XX_BLTC` | `./drivers/leds` |
| 84 | `ledtrig-pattern.ko` | Y | `CONFIG_LEDS_TRIGGER_PATTERN` | `./drivers/leds/trigger` |
| 85 | `mcdt_hw.ko` | Y | `CONFIG_SPRD_AUDIO_MCDT_R1P0` | `./drivers/vxtux/audio/mcdt/mcdt_r1p0` |
| 86 | `musb_hdrc.ko` | Y | `CONFIG_USB_MUSB_HDRC` | `./drivers/usb/musb` |
| 87 | `pinctrl-sprd.ko` | Y | `CONFIG_PINCTRL_SPRD` | `./drivers/pinctrl/sprd` |
| 88 | `pinctrl.ko` | Y | `CONFIG_PINCTRL` | `./drivers/base` |
| 89 | `pwm-sprd.ko` | Y | `CONFIG_PWM_SPRD` | `./drivers/pwm` |
| 90 | `sc27xx-poweroff.ko` | Y | `CONFIG_POWER_RESET_SC27XX` | `./drivers/power/reset` |
| 91 | `sc27xx-vibra.ko` | Y | `CONFIG_INPUT_SC27XX_VIBRA` | `./drivers/input/misc` |
| 92 | `sc27xx_fuel_gauge.ko` | Y | `CONFIG_FUEL_GAUGE_SC27XX` | `./drivers/power/supply` |
| 93 | `sipx.ko` | Y | `CONFIG_SPRD_SIPC_ZERO_COPY_SIPX` | `./drivers/soc/sprd/sipc` |
| 94 | `snd-soc-sprd-codec-sc2730-power-dev.ko` | Y | `CONFIG_VXTUX_SND_SPRD_SC2730_POWER` | `./drivers/vxtux/sound/asoc/codec` |
| 95 | `snd-soc-sprd-codec-sc2730-power.ko` | Y | `CONFIG_VXTUX_SND_SPRD_SC2730_POWER` | `./drivers/vxtux/sound/asoc/codec` |
| 96 | `snd-soc-sprd-codec-sc2730.ko` | Y | `CONFIG_VXTUX_SND_SPRD_SC2730` | `./drivers/vxtux/sound/asoc/codec_sc2730` |
| 97 | `spi-sprd-adi.ko` | Y | `CONFIG_SPI_SPRD_ADI` | `./drivers/spi` |
| 98 | `spipe.ko` | Y | `CONFIG_SPRD_SIPC_SPIPE` | `./drivers/soc/sprd/sipc` |
| 99 | `spool.ko` | Y | `CONFIG_SPRD_SIPC_SPOOL` | `./drivers/soc/sprd/sipc` |
| 100 | `sprd-charger-manager.ko` | Y | `CONFIG_CHARGER_SPRD_MANAGER` | `./drivers/power/supply/sprd` |
| 101 | `sprd-compr-2stage-dma.ko` | Y | `CONFIG_SND_SOC_SPRD` | `./sound/soc/sprd` |
| 102 | `sprd-dma.ko` | Y | `CONFIG_SPRD_DMA` | `./drivers/dma` |
| 103 | `sprd-iommu.ko` | Y | `CONFIG_SPRD_IOMMU` | `./drivers/iommu` |
| 104 | `sprd-ion.ko` | Y | `CONFIG_SPRD_ION` | `./drivers/dma-buf/sprd` |
| 105 | `sprd-platform-pcm-routing.ko` | Y | `CONFIG_VXTUX_SND_SPRD_PCM_ROUTING` | `./drivers/vxtux/sound/asoc/platform` |
| 106 | `sprd-sc27xx-spi.ko` | Y | `CONFIG_MFD_SC27XX_PMIC` | `./drivers/mfd` |
| 107 | `sprd-vsp-pw-domain.ko` | Y | `CONFIG_SPRD_VSP_PW_DOMAIN` | `./drivers/pmdomain/sprd` |
| 108 | `sprd_camsys_pw_domain.ko` | Y | `CONFIG_SPRD_CAMSYS_PW_DOMAIN` | `./drivers/pmdomain/sprd` |
| 109 | `sprd_camsys_pw_domain.ko` | Y | `CONFIG_SPRD_CAMSYS_PW_DOMAIN` | `./drivers/pmdomain/sprd` |
| 110 | `sprd_camsys_pw_domain.ko` | Y | `CONFIG_SPRD_CAMSYS_PW_DOMAIN` | `./drivers/pmdomain/sprd` |
| 111 | `sprd_cpp.ko` | Y | `CONFIG_SPRD_CPP` | `./drivers/media/platform/sprd/cpp` |
| 112 | `sprd_cpp.ko` | Y | `CONFIG_SPRD_CPP` | `./drivers/media/platform/sprd/cpp` |
| 113 | `sprd_hwspinlock.ko` | Y | `CONFIG_HWSPINLOCK_SPRD` | `./drivers/hwspinlock` |
| 114 | `sprd_ion.ko` | Y | `CONFIG_SPRD_ION` | `./drivers/dma-buf/sprd` |
| 115 | `sprd_jpg.ko` | Y | `CONFIG_SPRD_JPG` | `./drivers/misc/sprd_jpg` |
| 116 | `sprd_modem_loader.ko` | Y | `CONFIG_SPRD_MODEM_LOADER` | `./drivers/soc/sprd/modem/modem_loader` |
| 117 | `sprd_pmic_syscon.ko` | Y | `CONFIG_SPRD_PMIC_SYSCON` | `./drivers/soc/sprd/pmic_syscon` |
| 118 | `sprd_sensor.ko` | Y | `CONFIG_SPRD_SENSOR` | `./drivers/media/platform/sprd/sensors/sprd_sensor` |
| 119 | `sprd_soc_id.ko` | Y | `CONFIG_SPRD_SOCID` | `./drivers/soc/sprd/soc_id` |
| 120 | `sprd_vsp.ko` | Y | `CONFIG_SPRD_VSP` | `./drivers/media/platform/sprd/vsp` |
| 121 | `sprd_vsp_pw_domain.ko` | Y | `CONFIG_SPRD_VSP_PW_DOMAIN` | `./drivers/pmdomain/sprd` |
| 122 | `sprd_wdt_fiq.ko` | Y | `CONFIG_SPRD_WATCHDOG_FIQ` | `./drivers/watchdog` |
| 123 | `twofish_common.ko` | Y | `CONFIG_CRYPTO_TWOFISH_COMMON` | `./crypto` |
| 124 | `twofish_generic.ko` | Y | `CONFIG_CRYPTO_TWOFISH` | `./crypto` |
| 125 | `virt-dma.ko` | Y | `CONFIG_DMA_VIRTUAL_CHANNELS` | `./drivers/dma` |
| 126 | `vsp.ko` | Y | `CONFIG_SPRD_VSP` | `./drivers/media/platform/sprd/vsp` |
| 127 | `zram.ko` | Y | `CONFIG_ZRAM` | `./drivers/block/zram` |
| 128 | `zsmalloc.ko` | Y | `CONFIG_ZSMALLOC` | `./mm` |
| 129 | `gpio.ko` | OFF | `CONFIG_MTD_NAND_GPIO` | `./drivers/mtd/nand/raw` |
| 130 | `hxchipset_83102e.ko` | NOSRC | `` | `input/touchscreen/sprd/hx83102e/` |
| 131 | `i2c-sprd-hw.ko` | NOSRC | `` | `i2c/busses/` |
| 132 | `ion_cma_heap.ko` | NOSRC | `` | `dma-buf/heaps/cma_heap.c` |
| 133 | `misc_sprd_uid.ko` | NOSRC | `` | `soc/sprd/soc_id/sprd_uid.c` |
| 134 | `mmc_swcq.ko` | NOSRC | `` | `mmc/host/sdhci-sprd.c` |
| 135 | `musb_sprd.ko` | NOSRC | `` | `usb/musb/musb_sprd_vxtux.c` |
| 136 | `pinctrl-sprd-sharkl5Pro.ko` | NOSRC | `` | `pinctrl/sprd/pinctrl-sprd-sharkl5pro.c` |
| 137 | `sblock_bridge.ko` | NOSRC | `` | `soc/sprd/sipc/` |
| 138 | `sbuf_bridge.ko` | NOSRC | `` | `soc/sprd/sipc/` |
| 139 | `sha1-ce.ko` | NOSRC | `` | `crypto/sha1.c` |
| 140 | `sipc-core.ko` | NOSRC | `` | `soc/sprd/sipc/sipc.c` |
| 141 | `snd-soc-sprd-card.ko` | NOSRC | `` | `sound/soc/sprd/card/` |
| 142 | `snd-soc-sprd-dummy-codec.ko` | NOSRC | `` | `sound/soc/sprd/codec/dummy-codec.c` |
| 143 | `sprd-cpufreq-public.ko` | NOSRC | `` | `cpufreq/` |
| 144 | `sprd-cpufreq-v2.ko` | NOSRC | `` | `cpufreq/` |
| 145 | `sprd_7sreset.ko` | NOSRC | `` | `soc/sprd/` |
| 146 | `sprd_camera.ko` | NOSRC | `` | `media/platform/sprd/pipeline/sprd_dcam.c` |
| 147 | `sprd_iq.ko` | NOSRC | `` | `media/platform/sprd/` |
| 148 | `sprd_manufacturer_model.ko` | NOSRC | `` | `soc/sprd/` |
| 149 | `sprd_pdbg.ko` | NOSRC | `` | `soc/sprd/` |
| 150 | `sprd_power_stat.ko` | NOSRC | `` | `soc/sprd/` |
| 151 | `sprd_sip_svc.ko` | NOSRC | `` | `soc/sprd/` |
| 152 | `sprd_vdsp.ko` | NOSRC | `` | `913.568 B` |
| 153 | `unisoc-iommu.ko` | NOSRC | `` | `iommu/sprd-iommu.c` |
