# PORT AUDIT — 129 .ko vendor Advan vs tree 6.18.52-VXTux-UMS512

Dibuat otomatis oleh `donors/port_audit.py` (jangan edit manual).

Status: **KO** = ter-build .ko · **Y** = sumber ada + `=y` (built-in) · **OBJ-Y** = selalu built-in · **OFF** = sumber + Kconfig ada, belum diaktifkan · **NOKCONFIG** = Makefile pakai CONFIG_ tapi entri Kconfig hilang · **OFF-DESIGN** = sengaja `# ... is not set` · **AMBIGUOUS** = nama modul generik, banyak kandidat CONFIG_ (identitas harus dibuktikan dari blob, bukan dari nama) · **NOSRC** = sumber driver tidak ada di tree (butuh port kode baru)

## Ringkasan

| Status | Jumlah |
|---|---:|
| KO | 68 |
| Y | 47 |
| NOSRC | 14 |
| **total** | **129** |

## Tabel

| # | Driver | Status | Simbol | Sumber |
|---:|---|---|---|---|
| 1 | `aes-ce-ccm.ko` | KO | `CONFIG_CRYPTO_AES_ARM64_CE_CCM` | `./arch/arm64/crypto` |
| 2 | `aes-neon-blk.ko` | KO | `CONFIG_CRYPTO_AES_ARM64_NEON_BLK` | `./arch/arm64/crypto` |
| 3 | `agdsp_access.ko` | KO | `CONFIG_SPRD_AUDIO_AGDSP_ACCESS` | `./drivers/vxtux/audio/agdsp_access` |
| 4 | `apsys-dvfs.ko` | KO | `CONFIG_SPRD_APSYS_DVFS` | `./drivers/devfreq/sprd/apsys` |
| 5 | `asix.ko` | KO | `CONFIG_USB_NET_AX8817X` | `./drivers/net/usb` |
| 6 | `audio-dsp-dump.ko` | KO | `CONFIG_SPRD_AUDIO_DSP_DUMP` | `./drivers/vxtux/audio/audiodspdump` |
| 7 | `audio-pipe.ko` | KO | `CONFIG_SPRD_AUDIO_PIPE` | `./drivers/vxtux/audio/audiosipc` |
| 8 | `audio_sipc.ko` | KO | `CONFIG_SPRD_AUDIO_SIPC` | `./drivers/vxtux/audio/audiosipc` |
| 9 | `ax88179_178a.ko` | KO | `CONFIG_USB_NET_AX88179_178A` | `./drivers/net/usb` |
| 10 | `clk-sprd.ko` | KO | `CONFIG_SPRD_COMMON_CLK` | `./drivers/clk/sprd` |
| 11 | `core.ko` | KO | `CONFIG_SPRD_AUTOTEST` | `./drivers/vxtux/soc/autotest` |
| 12 | `extcon-usb-gpio.ko` | KO | `CONFIG_EXTCON_USB_GPIO` | `./drivers/extcon` |
| 13 | `gpio-pmic-eic-sprd.ko` | KO | `CONFIG_GPIO_PMIC_EIC_SPRD` | `./drivers/gpio` |
| 14 | `gpio.ko` | KO | `CONFIG_MTD_NAND_GPIO` | `./drivers/mtd/nand/raw` |
| 15 | `ion_ipc_trusty.ko` | KO | `CONFIG_ION_IPC_TRUSTY` | `./drivers/dma-buf/sprd` |
| 16 | `mipi_driver.ko` | KO | `CONFIG_SPRD_MIPI_LOG_R5P0` | `./drivers/unisoc_platform/debug_log/sharkl5` |
| 17 | `mmc_hsq.ko` | KO | `CONFIG_MMC_HSQ` | `./drivers/mmc/host` |
| 18 | `mmc_swcq.ko` | KO | `CONFIG_MMC_SWCQ` | `./drivers/mmc/host` |
| 19 | `mmdvfs.ko` | KO | `CONFIG_SPRD_MM_DVFS` | `./drivers/devfreq/sprd/mm` |
| 20 | `novatek_ts_spi.ko` | KO | `CONFIG_TOUCHSCREEN_NOVATEK_NVT_TS_SPI` | `./drivers/input/touchscreen/novatek_nvt_ts_spi` |
| 21 | `nvmem-sc27xx-efuse.ko` | KO | `CONFIG_NVMEM_SC27XX_EFUSE` | `./drivers/nvmem` |
| 22 | `nvmem_sprd_cache_efuse.ko` | KO | `CONFIG_NVMEM_SPRD_CACHE_EFUSE` | `./drivers/vxtux/soc/cache_efuse` |
| 23 | `nvmem_sprd_efuse.ko` | KO | `CONFIG_NVMEM_SPRD_EFUSE` | `./drivers/nvmem` |
| 24 | `phy-sprd-sharkl5Pro.ko` | KO | `CONFIG_PHY_SPRD_UMS512_USB2` | `./drivers/phy/sprd` |
| 25 | `pwm_bl.ko` | KO | `CONFIG_BACKLIGHT_PWM` | `./drivers/video/backlight` |
| 26 | `rtc-sc27xx.ko` | KO | `CONFIG_RTC_DRV_SC27XX` | `./drivers/rtc` |
| 27 | `sc2730-regulator.ko` | KO | `CONFIG_REGULATOR_SC2730` | `./drivers/regulator` |
| 28 | `sc27xx_adc.ko` | KO | `CONFIG_SC27XX_ADC` | `./drivers/iio/adc` |
| 29 | `sc27xx_typec.ko` | KO | `CONFIG_TYPEC_SPRD` | `./drivers/usb/typec/sprd` |
| 30 | `sdhci-sprd.ko` | KO | `CONFIG_MMC_SDHCI_SPRD` | `./drivers/mmc/host` |
| 31 | `sensorhub.ko` | KO | `CONFIG_IIO_SSP_SENSORHUB` | `./drivers/iio/common/ssp_sensors` |
| 32 | `slog_bridge.ko` | KO | `CONFIG_SPRD_SIPC_SLOG_BRIDGE` | `./drivers/vxtux/sipc` |
| 33 | `snd-soc-sprd-vbc-fe.ko` | KO | `CONFIG_VXTUX_SND_SPRD_VBC_V4` | `./drivers/vxtux/sound/asoc/dai_v4` |
| 34 | `snd-soc-sprd-vbc-v4.ko` | KO | `CONFIG_VXTUX_SND_SPRD_VBC_V4` | `./drivers/vxtux/sound/asoc/dai_v4` |
| 35 | `spi-sprd.ko` | KO | `CONFIG_SPI_SPRD` | `./drivers/spi` |
| 36 | `sprd-cpufreq-public.ko` | KO | `CONFIG_ARM_SPRD_CPUFREQ_PUBLIC` | `./drivers/cpufreq` |
| 37 | `sprd-cpufreq-v2.ko` | KO | `CONFIG_ARM_SPRD_CPUFREQ_V2` | `./drivers/cpufreq` |
| 38 | `sprd-dmaengine-pcm.ko` | KO | `CONFIG_VXTUX_SND_SPRD_DMAENGINE_PCM` | `./drivers/vxtux/sound/asoc/platform` |
| 39 | `sprd-drm.ko` | KO | `CONFIG_DRM_SPRD` | `./drivers/gpu/drm/sprd` |
| 40 | `sprd-gsp.ko` | KO | `CONFIG_DRM_SPRD_GSP` | `./drivers/gpu/drm/sprd/gsp` |
| 41 | `sprd-sipc-virt-bus.ko` | KO | `CONFIG_SPRD_SIPC_VIRT_BUS` | `./drivers/vxtux/sipc` |
| 42 | `sprd_7sreset.ko` | KO | `CONFIG_SPRD_7SRESET` | `./drivers/vxtux/soc/sprd_7sreset` |
| 43 | `sprd_apipe.ko` | KO | `CONFIG_SPRD_APIPE` | `./drivers/vxtux/audio/audio_pipe` |
| 44 | `sprd_audcp_boot.ko` | KO | `CONFIG_SPRD_AUDIO_CPBOOT` | `./drivers/vxtux/audio/audiocpboot` |
| 45 | `sprd_audcp_dvfs.ko` | KO | `CONFIG_SPRD_HW_DEVICE_DVFS_AGCP` | `./drivers/vxtux/audio/audiodvfs` |
| 46 | `sprd_battery_info.ko` | KO | `CONFIG_SPRD_BATTERY_INFO` | `./drivers/power/supply/sprd` |
| 47 | `sprd_flash_drv.ko` | KO | `CONFIG_SPRD_FLASH_DRV` | `./drivers/sprd/flash` |
| 48 | `sprd_iq.ko` | KO | `CONFIG_SPRD_IQ` | `./drivers/vxtux/soc/iq` |
| 49 | `sprd_manufacturer_model.ko` | KO | `CONFIG_SPRD_MANUFACTURER_MODEL` | `./drivers/vxtux/soc/manufacturer_model` |
| 50 | `sprd_mipi.ko` | KO | `CONFIG_SPRD_MODEM_DEBUG` | `./drivers/unisoc_platform/debug_log` |
| 51 | `sprd_pdbg.ko` | KO | `CONFIG_SPRD_PDBG` | `./drivers/vxtux/soc/pdbg` |
| 52 | `sprd_power_manager.ko` | KO | `CONFIG_SPRD_POWER_MANAGER` | `./drivers/soc/sprd/power_manager` |
| 53 | `sprd_power_stat.ko` | KO | `CONFIG_SPRD_DEBUGSTAT` | `./drivers/vxtux/soc/debugstat` |
| 54 | `sprd_shm.ko` | KO | `CONFIG_SPRD_TSHM` | `./drivers/trusty` |
| 55 | `sprd_sip_svc.ko` | KO | `CONFIG_SPRD_SIP_SVC` | `./drivers/vxtux/soc/sip_svc` |
| 56 | `sprd_systimer.ko` | KO | `CONFIG_SPRD_SYSTIMER` | `./drivers/clocksource` |
| 57 | `sprd_time_sync.ko` | KO | `CONFIG_SPRD_TIME_SYNC` | `./drivers/soc/sprd/time_sync` |
| 58 | `sprd_time_sync_cp.ko` | KO | `CONFIG_SPRD_TIME_SYNC_CP` | `./drivers/soc/sprd/time_sync` |
| 59 | `trusty-ipc.ko` | KO | `CONFIG_TRUSTY_VIRTIO_IPC` | `./drivers/trusty` |
| 60 | `trusty-irq.ko` | KO | `CONFIG_TRUSTY` | `./drivers/trusty` |
| 61 | `trusty-log.ko` | KO | `CONFIG_TRUSTY_LOG` | `./drivers/trusty` |
| 62 | `trusty-pm.ko` | KO | `CONFIG_TRUSTY` | `./drivers/trusty` |
| 63 | `trusty-tui.ko` | KO | `CONFIG_TRUSTY_TUI` | `./drivers/misc` |
| 64 | `trusty-virtio.ko` | KO | `CONFIG_TRUSTY_VIRTIO` | `./drivers/trusty` |
| 65 | `trusty.ko` | KO | `CONFIG_TRUSTY` | `./drivers/trusty` |
| 66 | `ums512-clk.ko` | KO | `CONFIG_SPRD_UMS512_CLK` | `./drivers/clk/sprd` |
| 67 | `unisoc-mailbox.ko` | KO | `CONFIG_SPRD_MBOX` | `./drivers/mailbox` |
| 68 | `usb_f_vser.ko` | KO | `CONFIG_USB_F_VSERIAL` | `./drivers/usb/gadget/function` |
| 69 | `arc4.ko` | Y | `CONFIG_CRYPTO_ARC4` | `./crypto` |
| 70 | `audio_mem.ko` | Y | `CONFIG_SPRD_AUDIO_MEMORY_MANAGER` | `./drivers/vxtux/audio/audiomem` |
| 71 | `bq2560x-charger.ko` | Y | `CONFIG_CHARGER_BQ256XX` | `./drivers/power/supply` |
| 72 | `ghash-ce.ko` | Y | `CONFIG_CRYPTO_GHASH_ARM64_CE` | `./arch/arm64/crypto` |
| 73 | `gpio-eic-sprd.ko` | Y | `CONFIG_GPIO_EIC_SPRD` | `./drivers/gpio` |
| 74 | `gpio-sprd.ko` | Y | `CONFIG_GPIO_SPRD` | `./drivers/gpio` |
| 75 | `i2c-sprd.ko` | Y | `CONFIG_I2C_SPRD` | `./drivers/i2c/busses` |
| 76 | `jpg.ko` | Y | `CONFIG_SPRD_JPG` | `./drivers/misc/sprd_jpg` |
| 77 | `kfifo_buf.ko` | Y | `CONFIG_IIO_KFIFO_BUF` | `./drivers/iio/buffer` |
| 78 | `leds-sc27xx-bltc.ko` | Y | `CONFIG_LEDS_SC27XX_BLTC` | `./drivers/leds` |
| 79 | `ledtrig-pattern.ko` | Y | `CONFIG_LEDS_TRIGGER_PATTERN` | `./drivers/leds/trigger` |
| 80 | `mcdt_hw.ko` | Y | `CONFIG_SPRD_AUDIO_MCDT_R1P0` | `./drivers/vxtux/audio/mcdt/mcdt_r1p0` |
| 81 | `musb_hdrc.ko` | Y | `CONFIG_USB_MUSB_HDRC` | `./drivers/usb/musb` |
| 82 | `pinctrl-sprd.ko` | Y | `CONFIG_PINCTRL_SPRD` | `./drivers/pinctrl/sprd` |
| 83 | `pinctrl.ko` | Y | `CONFIG_PINCTRL` | `./drivers/base` |
| 84 | `pwm-sprd.ko` | Y | `CONFIG_PWM_SPRD` | `./drivers/pwm` |
| 85 | `sc27xx-poweroff.ko` | Y | `CONFIG_POWER_RESET_SC27XX` | `./drivers/power/reset` |
| 86 | `sc27xx-vibra.ko` | Y | `CONFIG_INPUT_SC27XX_VIBRA` | `./drivers/input/misc` |
| 87 | `sc27xx_fuel_gauge.ko` | Y | `CONFIG_FUEL_GAUGE_SC27XX` | `./drivers/power/supply` |
| 88 | `sipx.ko` | Y | `CONFIG_SPRD_SIPC_ZERO_COPY_SIPX` | `./drivers/soc/sprd/sipc` |
| 89 | `snd-soc-sprd-codec-sc2730-power-dev.ko` | Y | `CONFIG_VXTUX_SND_SPRD_SC2730_POWER` | `./drivers/vxtux/sound/asoc/codec` |
| 90 | `snd-soc-sprd-codec-sc2730-power.ko` | Y | `CONFIG_VXTUX_SND_SPRD_SC2730_POWER` | `./drivers/vxtux/sound/asoc/codec` |
| 91 | `snd-soc-sprd-codec-sc2730.ko` | Y | `CONFIG_VXTUX_SND_SPRD_SC2730` | `./drivers/vxtux/sound/asoc/codec_sc2730` |
| 92 | `spi-sprd-adi.ko` | Y | `CONFIG_SPI_SPRD_ADI` | `./drivers/spi` |
| 93 | `spipe.ko` | Y | `CONFIG_SPRD_SIPC_SPIPE` | `./drivers/soc/sprd/sipc` |
| 94 | `spool.ko` | Y | `CONFIG_SPRD_SIPC_SPOOL` | `./drivers/soc/sprd/sipc` |
| 95 | `sprd-charger-manager.ko` | Y | `CONFIG_CHARGER_SPRD_MANAGER` | `./drivers/power/supply/sprd` |
| 96 | `sprd-compr-2stage-dma.ko` | Y | `CONFIG_SND_SOC_SPRD` | `./sound/soc/sprd` |
| 97 | `sprd-dma.ko` | Y | `CONFIG_SPRD_DMA` | `./drivers/dma` |
| 98 | `sprd-ion.ko` | Y | `CONFIG_SPRD_ION` | `./drivers/dma-buf/sprd` |
| 99 | `sprd-platform-pcm-routing.ko` | Y | `CONFIG_VXTUX_SND_SPRD_PCM_ROUTING` | `./drivers/vxtux/sound/asoc/platform` |
| 100 | `sprd-sc27xx-spi.ko` | Y | `CONFIG_MFD_SC27XX_PMIC` | `./drivers/mfd` |
| 101 | `sprd-vsp-pw-domain.ko` | Y | `CONFIG_SPRD_VSP_PW_DOMAIN` | `./drivers/pmdomain/sprd` |
| 102 | `sprd_camsys_pw_domain.ko` | Y | `CONFIG_SPRD_CAMSYS_PW_DOMAIN` | `./drivers/pmdomain/sprd` |
| 103 | `sprd_cpp.ko` | Y | `CONFIG_SPRD_CPP` | `./drivers/media/platform/sprd/cpp` |
| 104 | `sprd_hwspinlock.ko` | Y | `CONFIG_HWSPINLOCK_SPRD` | `./drivers/hwspinlock` |
| 105 | `sprd_modem_loader.ko` | Y | `CONFIG_SPRD_MODEM_LOADER` | `./drivers/soc/sprd/modem/modem_loader` |
| 106 | `sprd_pmic_syscon.ko` | Y | `CONFIG_SPRD_PMIC_SYSCON` | `./drivers/soc/sprd/pmic_syscon` |
| 107 | `sprd_sensor.ko` | Y | `CONFIG_SPRD_SENSOR` | `./drivers/media/platform/sprd/sensors/sprd_sensor` |
| 108 | `sprd_soc_id.ko` | Y | `CONFIG_SPRD_SOCID` | `./drivers/soc/sprd/soc_id` |
| 109 | `sprd_wdt_fiq.ko` | Y | `CONFIG_SPRD_WATCHDOG_FIQ` | `./drivers/watchdog` |
| 110 | `twofish_common.ko` | Y | `CONFIG_CRYPTO_TWOFISH_COMMON` | `./crypto` |
| 111 | `twofish_generic.ko` | Y | `CONFIG_CRYPTO_TWOFISH` | `./crypto` |
| 112 | `virt-dma.ko` | Y | `CONFIG_DMA_VIRTUAL_CHANNELS` | `./drivers/dma` |
| 113 | `vsp.ko` | Y | `CONFIG_SPRD_VSP` | `./drivers/media/platform/sprd/vsp` |
| 114 | `zram.ko` | Y | `CONFIG_ZRAM` | `./drivers/block/zram` |
| 115 | `zsmalloc.ko` | Y | `CONFIG_ZSMALLOC` | `./mm` |
| 116 | `hxchipset_83102e.ko` | NOSRC | `` | `input/touchscreen/sprd/hx83102e/` |
| 117 | `i2c-sprd-hw.ko` | NOSRC | `` | `i2c/busses/` |
| 118 | `ion_cma_heap.ko` | NOSRC | `` | `dma-buf/heaps/cma_heap.c` |
| 119 | `misc_sprd_uid.ko` | NOSRC | `` | `soc/sprd/soc_id/sprd_uid.c` |
| 120 | `musb_sprd.ko` | NOSRC | `` | `usb/musb/musb_sprd_vxtux.c` |
| 121 | `pinctrl-sprd-sharkl5Pro.ko` | NOSRC | `` | `pinctrl/sprd/pinctrl-sprd-sharkl5pro.c` |
| 122 | `sblock_bridge.ko` | NOSRC | `` | `soc/sprd/sipc/` |
| 123 | `sbuf_bridge.ko` | NOSRC | `` | `soc/sprd/sipc/` |
| 124 | `sha1-ce.ko` | NOSRC | `` | `crypto/sha1.c` |
| 125 | `sipc-core.ko` | NOSRC | `` | `soc/sprd/sipc/sipc.c` |
| 126 | `snd-soc-sprd-card.ko` | NOSRC | `` | `sound/soc/sprd/card/` |
| 127 | `snd-soc-sprd-dummy-codec.ko` | NOSRC | `` | `sound/soc/sprd/codec/dummy-codec.c` |
| 128 | `sprd_camera.ko` | NOSRC | `` | `media/platform/sprd/pipeline/sprd_dcam.c` |
| 129 | `unisoc-iommu.ko` | NOSRC | `` | `iommu/sprd-iommu.c` |
