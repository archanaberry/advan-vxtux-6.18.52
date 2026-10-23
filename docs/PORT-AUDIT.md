# PORT AUDIT — 129 .ko vendor Advan vs tree 6.18.52-VXTux-UMS512

Dibuat otomatis oleh `donors/port_audit.py` (jangan edit manual).

Status: **KO** = ter-build .ko · **Y** = sumber ada + `=y` (built-in) · **OBJ-Y** = selalu built-in · **OFF** = sumber + Kconfig ada, belum diaktifkan · **NOKCONFIG** = Makefile pakai CONFIG_ tapi entri Kconfig hilang · **OFF-DESIGN** = sengaja `# ... is not set` · **AMBIGUOUS** = nama modul generik, banyak kandidat CONFIG_ (identitas harus dibuktikan dari blob, bukan dari nama) · **NOSRC** = sumber driver tidak ada di tree (butuh port kode baru)

## Ringkasan

| Status | Jumlah |
|---|---:|
| KO | 58 |
| Y | 47 |
| NOSRC | 24 |
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
| 11 | `extcon-usb-gpio.ko` | KO | `CONFIG_EXTCON_USB_GPIO` | `./drivers/extcon` |
| 12 | `gpio-pmic-eic-sprd.ko` | KO | `CONFIG_GPIO_PMIC_EIC_SPRD` | `./drivers/gpio` |
| 13 | `ion_ipc_trusty.ko` | KO | `CONFIG_ION_IPC_TRUSTY` | `./drivers/dma-buf/sprd` |
| 14 | `mipi_driver.ko` | KO | `CONFIG_SPRD_MIPI_LOG_R5P0` | `./drivers/unisoc_platform/debug_log/sharkl5` |
| 15 | `mmc_hsq.ko` | KO | `CONFIG_MMC_HSQ` | `./drivers/mmc/host` |
| 16 | `mmdvfs.ko` | KO | `CONFIG_SPRD_MM_DVFS` | `./drivers/devfreq/sprd/mm` |
| 17 | `novatek_ts_spi.ko` | KO | `CONFIG_TOUCHSCREEN_NOVATEK_NVT_TS_SPI` | `./drivers/input/touchscreen/novatek_nvt_ts_spi` |
| 18 | `nvmem-sc27xx-efuse.ko` | KO | `CONFIG_NVMEM_SC27XX_EFUSE` | `./drivers/nvmem` |
| 19 | `nvmem_sprd_cache_efuse.ko` | KO | `CONFIG_NVMEM_SPRD_CACHE_EFUSE` | `./drivers/vxtux/soc/cache_efuse` |
| 20 | `nvmem_sprd_efuse.ko` | KO | `CONFIG_NVMEM_SPRD_EFUSE` | `./drivers/nvmem` |
| 21 | `phy-sprd-sharkl5Pro.ko` | KO | `CONFIG_PHY_SPRD_UMS512_USB2` | `./drivers/phy/sprd` |
| 22 | `pwm_bl.ko` | KO | `CONFIG_BACKLIGHT_PWM` | `./drivers/video/backlight` |
| 23 | `rtc-sc27xx.ko` | KO | `CONFIG_RTC_DRV_SC27XX` | `./drivers/rtc` |
| 24 | `sc2730-regulator.ko` | KO | `CONFIG_REGULATOR_SC2730` | `./drivers/regulator` |
| 25 | `sc27xx_adc.ko` | KO | `CONFIG_SC27XX_ADC` | `./drivers/iio/adc` |
| 26 | `sc27xx_typec.ko` | KO | `CONFIG_TYPEC_SPRD` | `./drivers/usb/typec/sprd` |
| 27 | `sdhci-sprd.ko` | KO | `CONFIG_MMC_SDHCI_SPRD` | `./drivers/mmc/host` |
| 28 | `sensorhub.ko` | KO | `CONFIG_IIO_SSP_SENSORHUB` | `./drivers/iio/common/ssp_sensors` |
| 29 | `slog_bridge.ko` | KO | `CONFIG_SPRD_SIPC_SLOG_BRIDGE` | `./drivers/vxtux/sipc` |
| 30 | `snd-soc-sprd-vbc-fe.ko` | KO | `CONFIG_VXTUX_SND_SPRD_VBC_V4` | `./drivers/vxtux/sound/asoc/dai_v4` |
| 31 | `snd-soc-sprd-vbc-v4.ko` | KO | `CONFIG_VXTUX_SND_SPRD_VBC_V4` | `./drivers/vxtux/sound/asoc/dai_v4` |
| 32 | `spi-sprd.ko` | KO | `CONFIG_SPI_SPRD` | `./drivers/spi` |
| 33 | `sprd-dmaengine-pcm.ko` | KO | `CONFIG_VXTUX_SND_SPRD_DMAENGINE_PCM` | `./drivers/vxtux/sound/asoc/platform` |
| 34 | `sprd-drm.ko` | KO | `CONFIG_DRM_SPRD` | `./drivers/gpu/drm/sprd` |
| 35 | `sprd-gsp.ko` | KO | `CONFIG_DRM_SPRD_GSP` | `./drivers/gpu/drm/sprd/gsp` |
| 36 | `sprd-sipc-virt-bus.ko` | KO | `CONFIG_SPRD_SIPC_VIRT_BUS` | `./drivers/vxtux/sipc` |
| 37 | `sprd_apipe.ko` | KO | `CONFIG_SPRD_APIPE` | `./drivers/vxtux/audio/audio_pipe` |
| 38 | `sprd_audcp_boot.ko` | KO | `CONFIG_SPRD_AUDIO_CPBOOT` | `./drivers/vxtux/audio/audiocpboot` |
| 39 | `sprd_audcp_dvfs.ko` | KO | `CONFIG_SPRD_HW_DEVICE_DVFS_AGCP` | `./drivers/vxtux/audio/audiodvfs` |
| 40 | `sprd_battery_info.ko` | KO | `CONFIG_SPRD_BATTERY_INFO` | `./drivers/power/supply/sprd` |
| 41 | `sprd_flash_drv.ko` | KO | `CONFIG_SPRD_FLASH_DRV` | `./drivers/sprd/flash` |
| 42 | `sprd_mipi.ko` | KO | `CONFIG_SPRD_MODEM_DEBUG` | `./drivers/unisoc_platform/debug_log` |
| 43 | `sprd_power_manager.ko` | KO | `CONFIG_SPRD_POWER_MANAGER` | `./drivers/soc/sprd/power_manager` |
| 44 | `sprd_shm.ko` | KO | `CONFIG_SPRD_TSHM` | `./drivers/trusty` |
| 45 | `sprd_sip_svc.ko` | KO | `CONFIG_SPRD_SIP_SVC` | `./drivers/vxtux/soc/sip_svc` |
| 46 | `sprd_systimer.ko` | KO | `CONFIG_SPRD_SYSTIMER` | `./drivers/clocksource` |
| 47 | `sprd_time_sync.ko` | KO | `CONFIG_SPRD_TIME_SYNC` | `./drivers/soc/sprd/time_sync` |
| 48 | `sprd_time_sync_cp.ko` | KO | `CONFIG_SPRD_TIME_SYNC_CP` | `./drivers/soc/sprd/time_sync` |
| 49 | `trusty-ipc.ko` | KO | `CONFIG_TRUSTY_VIRTIO_IPC` | `./drivers/trusty` |
| 50 | `trusty-irq.ko` | KO | `CONFIG_TRUSTY` | `./drivers/trusty` |
| 51 | `trusty-log.ko` | KO | `CONFIG_TRUSTY_LOG` | `./drivers/trusty` |
| 52 | `trusty-pm.ko` | KO | `CONFIG_TRUSTY` | `./drivers/trusty` |
| 53 | `trusty-tui.ko` | KO | `CONFIG_TRUSTY_TUI` | `./drivers/misc` |
| 54 | `trusty-virtio.ko` | KO | `CONFIG_TRUSTY_VIRTIO` | `./drivers/trusty` |
| 55 | `trusty.ko` | KO | `CONFIG_TRUSTY` | `./drivers/trusty` |
| 56 | `ums512-clk.ko` | KO | `CONFIG_SPRD_UMS512_CLK` | `./drivers/clk/sprd` |
| 57 | `unisoc-mailbox.ko` | KO | `CONFIG_SPRD_MBOX` | `./drivers/mailbox` |
| 58 | `usb_f_vser.ko` | KO | `CONFIG_USB_F_VSERIAL` | `./drivers/usb/gadget/function` |
| 59 | `arc4.ko` | Y | `CONFIG_CRYPTO_ARC4` | `./crypto` |
| 60 | `audio_mem.ko` | Y | `CONFIG_SPRD_AUDIO_MEMORY_MANAGER` | `./drivers/vxtux/audio/audiomem` |
| 61 | `bq2560x-charger.ko` | Y | `CONFIG_CHARGER_BQ256XX` | `./drivers/power/supply` |
| 62 | `ghash-ce.ko` | Y | `CONFIG_CRYPTO_GHASH_ARM64_CE` | `./arch/arm64/crypto` |
| 63 | `gpio-eic-sprd.ko` | Y | `CONFIG_GPIO_EIC_SPRD` | `./drivers/gpio` |
| 64 | `gpio-sprd.ko` | Y | `CONFIG_GPIO_SPRD` | `./drivers/gpio` |
| 65 | `i2c-sprd.ko` | Y | `CONFIG_I2C_SPRD` | `./drivers/i2c/busses` |
| 66 | `jpg.ko` | Y | `CONFIG_SPRD_JPG` | `./drivers/misc/sprd_jpg` |
| 67 | `kfifo_buf.ko` | Y | `CONFIG_IIO_KFIFO_BUF` | `./drivers/iio/buffer` |
| 68 | `leds-sc27xx-bltc.ko` | Y | `CONFIG_LEDS_SC27XX_BLTC` | `./drivers/leds` |
| 69 | `ledtrig-pattern.ko` | Y | `CONFIG_LEDS_TRIGGER_PATTERN` | `./drivers/leds/trigger` |
| 70 | `mcdt_hw.ko` | Y | `CONFIG_SPRD_AUDIO_MCDT_R1P0` | `./drivers/vxtux/audio/mcdt/mcdt_r1p0` |
| 71 | `musb_hdrc.ko` | Y | `CONFIG_USB_MUSB_HDRC` | `./drivers/usb/musb` |
| 72 | `pinctrl-sprd.ko` | Y | `CONFIG_PINCTRL_SPRD` | `./drivers/pinctrl/sprd` |
| 73 | `pinctrl.ko` | Y | `CONFIG_PINCTRL` | `./drivers/base` |
| 74 | `pwm-sprd.ko` | Y | `CONFIG_PWM_SPRD` | `./drivers/pwm` |
| 75 | `sc27xx-poweroff.ko` | Y | `CONFIG_POWER_RESET_SC27XX` | `./drivers/power/reset` |
| 76 | `sc27xx-vibra.ko` | Y | `CONFIG_INPUT_SC27XX_VIBRA` | `./drivers/input/misc` |
| 77 | `sc27xx_fuel_gauge.ko` | Y | `CONFIG_FUEL_GAUGE_SC27XX` | `./drivers/power/supply` |
| 78 | `sipx.ko` | Y | `CONFIG_SPRD_SIPC_ZERO_COPY_SIPX` | `./drivers/soc/sprd/sipc` |
| 79 | `snd-soc-sprd-codec-sc2730-power-dev.ko` | Y | `CONFIG_VXTUX_SND_SPRD_SC2730_POWER` | `./drivers/vxtux/sound/asoc/codec` |
| 80 | `snd-soc-sprd-codec-sc2730-power.ko` | Y | `CONFIG_VXTUX_SND_SPRD_SC2730_POWER` | `./drivers/vxtux/sound/asoc/codec` |
| 81 | `snd-soc-sprd-codec-sc2730.ko` | Y | `CONFIG_VXTUX_SND_SPRD_SC2730` | `./drivers/vxtux/sound/asoc/codec_sc2730` |
| 82 | `spi-sprd-adi.ko` | Y | `CONFIG_SPI_SPRD_ADI` | `./drivers/spi` |
| 83 | `spipe.ko` | Y | `CONFIG_SPRD_SIPC_SPIPE` | `./drivers/soc/sprd/sipc` |
| 84 | `spool.ko` | Y | `CONFIG_SPRD_SIPC_SPOOL` | `./drivers/soc/sprd/sipc` |
| 85 | `sprd-charger-manager.ko` | Y | `CONFIG_CHARGER_SPRD_MANAGER` | `./drivers/power/supply/sprd` |
| 86 | `sprd-compr-2stage-dma.ko` | Y | `CONFIG_SND_SOC_SPRD` | `./sound/soc/sprd` |
| 87 | `sprd-dma.ko` | Y | `CONFIG_SPRD_DMA` | `./drivers/dma` |
| 88 | `sprd-ion.ko` | Y | `CONFIG_SPRD_ION` | `./drivers/dma-buf/sprd` |
| 89 | `sprd-platform-pcm-routing.ko` | Y | `CONFIG_VXTUX_SND_SPRD_PCM_ROUTING` | `./drivers/vxtux/sound/asoc/platform` |
| 90 | `sprd-sc27xx-spi.ko` | Y | `CONFIG_MFD_SC27XX_PMIC` | `./drivers/mfd` |
| 91 | `sprd-vsp-pw-domain.ko` | Y | `CONFIG_SPRD_VSP_PW_DOMAIN` | `./drivers/pmdomain/sprd` |
| 92 | `sprd_camsys_pw_domain.ko` | Y | `CONFIG_SPRD_CAMSYS_PW_DOMAIN` | `./drivers/pmdomain/sprd` |
| 93 | `sprd_cpp.ko` | Y | `CONFIG_SPRD_CPP` | `./drivers/media/platform/sprd/cpp` |
| 94 | `sprd_hwspinlock.ko` | Y | `CONFIG_HWSPINLOCK_SPRD` | `./drivers/hwspinlock` |
| 95 | `sprd_modem_loader.ko` | Y | `CONFIG_SPRD_MODEM_LOADER` | `./drivers/soc/sprd/modem/modem_loader` |
| 96 | `sprd_pmic_syscon.ko` | Y | `CONFIG_SPRD_PMIC_SYSCON` | `./drivers/soc/sprd/pmic_syscon` |
| 97 | `sprd_sensor.ko` | Y | `CONFIG_SPRD_SENSOR` | `./drivers/media/platform/sprd/sensors/sprd_sensor` |
| 98 | `sprd_soc_id.ko` | Y | `CONFIG_SPRD_SOCID` | `./drivers/soc/sprd/soc_id` |
| 99 | `sprd_wdt_fiq.ko` | Y | `CONFIG_SPRD_WATCHDOG_FIQ` | `./drivers/watchdog` |
| 100 | `twofish_common.ko` | Y | `CONFIG_CRYPTO_TWOFISH_COMMON` | `./crypto` |
| 101 | `twofish_generic.ko` | Y | `CONFIG_CRYPTO_TWOFISH` | `./crypto` |
| 102 | `virt-dma.ko` | Y | `CONFIG_DMA_VIRTUAL_CHANNELS` | `./drivers/dma` |
| 103 | `vsp.ko` | Y | `CONFIG_SPRD_VSP` | `./drivers/media/platform/sprd/vsp` |
| 104 | `zram.ko` | Y | `CONFIG_ZRAM` | `./drivers/block/zram` |
| 105 | `zsmalloc.ko` | Y | `CONFIG_ZSMALLOC` | `./mm` |
| 106 | `core.ko` | NOSRC | `` | `blob: sprd autotest core driver; framework SPRD_AUTOTEST tidak ada di tree` |
| 107 | `gpio.ko` | NOSRC | `` | `blob: sprd autotest gpio driver (depends=core); SPRD_AUTOTEST/SPRD_AUTOTEST_GPIO tidak ada di tree` |
| 108 | `hxchipset_83102e.ko` | NOSRC | `` | `335310` |
| 109 | `i2c-sprd-hw.ko` | NOSRC | `` | `33806` |
| 110 | `ion_cma_heap.ko` | NOSRC | `` | `34502` |
| 111 | `misc_sprd_uid.ko` | NOSRC | `` | `17990` |
| 112 | `mmc_swcq.ko` | NOSRC | `` | `72670` |
| 113 | `musb_sprd.ko` | NOSRC | `` | `65934` |
| 114 | `pinctrl-sprd-sharkl5Pro.ko` | NOSRC | `` | `49870` |
| 115 | `sblock_bridge.ko` | NOSRC | `` | `21150` |
| 116 | `sbuf_bridge.ko` | NOSRC | `` | `22334` |
| 117 | `sha1-ce.ko` | NOSRC | `` | `19558` |
| 118 | `sipc-core.ko` | NOSRC | `` | `230910` |
| 119 | `snd-soc-sprd-card.ko` | NOSRC | `` | `131502` |
| 120 | `snd-soc-sprd-dummy-codec.ko` | NOSRC | `` | `16806` |
| 121 | `sprd-cpufreq-public.ko` | NOSRC | `` | `17334` |
| 122 | `sprd-cpufreq-v2.ko` | NOSRC | `` | `56030` |
| 123 | `sprd_7sreset.ko` | NOSRC | `` | `23350` |
| 124 | `sprd_camera.ko` | NOSRC | `` | `2650982` |
| 125 | `sprd_iq.ko` | NOSRC | `` | `32566` |
| 126 | `sprd_manufacturer_model.ko` | NOSRC | `` | `16902` |
| 127 | `sprd_pdbg.ko` | NOSRC | `` | `40854` |
| 128 | `sprd_power_stat.ko` | NOSRC | `` | `42670` |
| 129 | `unisoc-iommu.ko` | NOSRC | `` | `122990` |
