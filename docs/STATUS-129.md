# Status 129 blob stok — sudah jalan di 6.18.52 atau belum

Dibuat otomatis [coverage_129.py](../donors/coverage_129.py) dari daftar `.ko` hasil build + config live.

- ✅ = fungsinya sudah tersedia di build 6.18.52 (`.ko` pengganti, kode ikut `Image`, atau fungsi sudah ada di modul lain)
- **(X)** = belum ada penggantinya

**129/129** ✅ · **0** (X)

| # | driver stok 5.4 | di 6.18.52 | catatan |
|---:|---|---|---|
| 1 | `aes-ce-ccm.ko` | ✅ `KO` | aes-ce-ccm.ko |
| 2 | `aes-neon-blk.ko` | ✅ `KO` | aes-neon-blk.ko |
| 3 | `agdsp_access.ko` | ✅ `KO` | agdsp_access.ko |
| 4 | `apsys-dvfs.ko` | ✅ `KO` | apsys-dvfs.ko |
| 5 | `arc4.ko` | ✅ `Y` | CONFIG_CRYPTO_ARC4 |
| 6 | `asix.ko` | ✅ `KO` | asix.ko |
| 7 | `audio-dsp-dump.ko` | ✅ `KO` | audio-dsp-dump.ko |
| 8 | `audio-pipe.ko` | ✅ `KO` | audio-pipe.ko |
| 9 | `audio_mem.ko` | ✅ `Y` | CONFIG_SPRD_AUDIO_MEMORY_MANAGER |
| 10 | `audio_sipc.ko` | ✅ `KO` | audio_sipc.ko |
| 11 | `ax88179_178a.ko` | ✅ `KO` | ax88179_178a.ko |
| 12 | `bq2560x-charger.ko` | ✅ `Y` | CONFIG_CHARGER_BQ256XX |
| 13 | `clk-sprd.ko` | ✅ `KO` | clk-sprd.ko |
| 14 | `core.ko` | ✅ `KO` | sprd_autotest_core.ko |
| 15 | `extcon-usb-gpio.ko` | ✅ `KO` | extcon-usb-gpio.ko |
| 16 | `ghash-ce.ko` | ✅ `Y` | CONFIG_CRYPTO_GHASH_ARM64_CE |
| 17 | `gpio-eic-sprd.ko` | ✅ `Y` | CONFIG_GPIO_EIC_SPRD |
| 18 | `gpio-pmic-eic-sprd.ko` | ✅ `KO` | gpio-pmic-eic-sprd.ko |
| 19 | `gpio-sprd.ko` | ✅ `Y` | CONFIG_GPIO_SPRD |
| 20 | `gpio.ko` | ✅ `KO` | sprd_autotest_gpio.ko |
| 21 | `hxchipset_83102e.ko` | ✅ `BUILT-IN` | triage + config pendukung =y |
| 22 | `i2c-sprd-hw.ko` | ✅ `BUILT-IN` | triage + config pendukung =y |
| 23 | `i2c-sprd.ko` | ✅ `Y` | CONFIG_I2C_SPRD |
| 24 | `ion_cma_heap.ko` | ✅ `BUILT-IN` | triage + config pendukung =y |
| 25 | `ion_ipc_trusty.ko` | ✅ `KO` | ion_ipc_trusty.ko |
| 26 | `jpg.ko` | ✅ `Y` | CONFIG_SPRD_JPG |
| 27 | `kfifo_buf.ko` | ✅ `Y` | CONFIG_IIO_KFIFO_BUF |
| 28 | `leds-sc27xx-bltc.ko` | ✅ `Y` | CONFIG_LEDS_SC27XX_BLTC |
| 29 | `ledtrig-pattern.ko` | ✅ `Y` | CONFIG_LEDS_TRIGGER_PATTERN |
| 30 | `mcdt_hw.ko` | ✅ `Y` | CONFIG_SPRD_AUDIO_MCDT_R1P0 |
| 31 | `mipi_driver.ko` | ✅ `KO` | mipi_driver.ko |
| 32 | `misc_sprd_uid.ko` | ✅ `BUILT-IN` | triage + config pendukung =y |
| 33 | `mmc_hsq.ko` | ✅ `KO` | mmc_hsq.ko |
| 34 | `mmc_swcq.ko` | ✅ `KO` | mmc_swcq.ko |
| 35 | `mmdvfs.ko` | ✅ `KO` | mmdvfs.ko |
| 36 | `musb_hdrc.ko` | ✅ `Y` | CONFIG_USB_MUSB_HDRC |
| 37 | `musb_sprd.ko` | ✅ `BUILT-IN` | triage + config pendukung =y |
| 38 | `novatek_ts_spi.ko` | ✅ `KO` | novatek_ts_spi.ko |
| 39 | `nvmem-sc27xx-efuse.ko` | ✅ `KO` | nvmem-sc27xx-efuse.ko |
| 40 | `nvmem_sprd_cache_efuse.ko` | ✅ `KO` | nvmem_sprd_cache_efuse.ko |
| 41 | `nvmem_sprd_efuse.ko` | ✅ `KO` | nvmem_sprd_efuse.ko |
| 42 | `phy-sprd-sharkl5Pro.ko` | ✅ `KO` | phy-sprd-ums512.ko |
| 43 | `pinctrl-sprd-sharkl5Pro.ko` | ✅ `BUILT-IN` | triage + config pendukung =y |
| 44 | `pinctrl-sprd.ko` | ✅ `Y` | CONFIG_PINCTRL_SPRD |
| 45 | `pinctrl.ko` | ✅ `Y` | CONFIG_PINCTRL |
| 46 | `pwm-sprd.ko` | ✅ `Y` | CONFIG_PWM_SPRD |
| 47 | `pwm_bl.ko` | ✅ `KO` | pwm_bl.ko |
| 48 | `rtc-sc27xx.ko` | ✅ `KO` | rtc-sc27xx.ko |
| 49 | `sblock_bridge.ko` | ✅ `BUILT-IN` | triage + config pendukung =y |
| 50 | `sbuf_bridge.ko` | ✅ `BUILT-IN` | triage + config pendukung =y |
| 51 | `sc2730-regulator.ko` | ✅ `KO` | sc2730-regulator.ko |
| 52 | `sc27xx-poweroff.ko` | ✅ `Y` | CONFIG_POWER_RESET_SC27XX |
| 53 | `sc27xx-vibra.ko` | ✅ `Y` | CONFIG_INPUT_SC27XX_VIBRA |
| 54 | `sc27xx_adc.ko` | ✅ `KO` | sc27xx_adc.ko |
| 55 | `sc27xx_fuel_gauge.ko` | ✅ `Y` | CONFIG_FUEL_GAUGE_SC27XX |
| 56 | `sc27xx_typec.ko` | ✅ `KO` | sc27xx_typec.ko |
| 57 | `sdhci-sprd.ko` | ✅ `KO` | sdhci-sprd.ko |
| 58 | `sensorhub.ko` | ✅ `KO` | sensorhub.ko |
| 59 | `sha1-ce.ko` | ✅ `SUPERSEDED` | fungsinya ada di modul lain |
| 60 | `sipc-core.ko` | ✅ `BUILT-IN` | triage + config pendukung =y |
| 61 | `sipx.ko` | ✅ `Y` | CONFIG_SPRD_SIPC_ZERO_COPY_SIPX |
| 62 | `slog_bridge.ko` | ✅ `KO` | slog_bridge.ko |
| 63 | `snd-soc-sprd-card.ko` | ✅ `SUPERSEDED` | fungsinya ada di modul lain |
| 64 | `snd-soc-sprd-codec-sc2730-power-dev.ko` | ✅ `Y` | CONFIG_VXTUX_SND_SPRD_SC2730_POWER |
| 65 | `snd-soc-sprd-codec-sc2730-power.ko` | ✅ `Y` | CONFIG_VXTUX_SND_SPRD_SC2730_POWER |
| 66 | `snd-soc-sprd-codec-sc2730.ko` | ✅ `Y` | CONFIG_VXTUX_SND_SPRD_SC2730 |
| 67 | `snd-soc-sprd-dummy-codec.ko` | ✅ `BUILT-IN` | triage + config pendukung =y |
| 68 | `snd-soc-sprd-vbc-fe.ko` | ✅ `KO` | snd-soc-sprd-vbc-v4.ko |
| 69 | `snd-soc-sprd-vbc-v4.ko` | ✅ `KO` | snd-soc-sprd-vbc-v4.ko |
| 70 | `spi-sprd-adi.ko` | ✅ `Y` | CONFIG_SPI_SPRD_ADI |
| 71 | `spi-sprd.ko` | ✅ `KO` | spi-sprd.ko |
| 72 | `spipe.ko` | ✅ `Y` | CONFIG_SPRD_SIPC_SPIPE |
| 73 | `spool.ko` | ✅ `Y` | CONFIG_SPRD_SIPC_SPOOL |
| 74 | `sprd-charger-manager.ko` | ✅ `Y` | CONFIG_CHARGER_SPRD_MANAGER |
| 75 | `sprd-compr-2stage-dma.ko` | ✅ `Y` | CONFIG_SND_SOC_SPRD |
| 76 | `sprd-cpufreq-public.ko` | ✅ `KO` | sprd-cpufreq-public.ko |
| 77 | `sprd-cpufreq-v2.ko` | ✅ `KO` | sprd-cpufreq-v2.ko |
| 78 | `sprd-dma.ko` | ✅ `Y` | CONFIG_SPRD_DMA |
| 79 | `sprd-dmaengine-pcm.ko` | ✅ `KO` | sprd-dmaengine-pcm.ko |
| 80 | `sprd-drm.ko` | ✅ `KO` | sprd-drm.ko |
| 81 | `sprd-gsp.ko` | ✅ `KO` | sprd-gsp.ko |
| 82 | `sprd-ion.ko` | ✅ `Y` | CONFIG_SPRD_ION |
| 83 | `sprd-platform-pcm-routing.ko` | ✅ `Y` | CONFIG_VXTUX_SND_SPRD_PCM_ROUTING |
| 84 | `sprd-sc27xx-spi.ko` | ✅ `Y` | CONFIG_MFD_SC27XX_PMIC |
| 85 | `sprd-sipc-virt-bus.ko` | ✅ `KO` | sprd-sipc-virt-bus.ko |
| 86 | `sprd-vsp-pw-domain.ko` | ✅ `Y` | CONFIG_SPRD_VSP_PW_DOMAIN |
| 87 | `sprd_7sreset.ko` | ✅ `KO` | sprd_7sreset.ko |
| 88 | `sprd_apipe.ko` | ✅ `KO` | sprd_apipe.ko |
| 89 | `sprd_audcp_boot.ko` | ✅ `KO` | sprd_audcp_boot.ko |
| 90 | `sprd_audcp_dvfs.ko` | ✅ `KO` | sprd_audcp_dvfs.ko |
| 91 | `sprd_battery_info.ko` | ✅ `KO` | sprd_battery_info.ko |
| 92 | `sprd_camera.ko` | ✅ `BUILT-IN` | triage + config pendukung =y |
| 93 | `sprd_camsys_pw_domain.ko` | ✅ `Y` | CONFIG_SPRD_CAMSYS_PW_DOMAIN |
| 94 | `sprd_cpp.ko` | ✅ `Y` | CONFIG_SPRD_CPP |
| 95 | `sprd_flash_drv.ko` | ✅ `KO` | sprd_flash_drv.ko |
| 96 | `sprd_hwspinlock.ko` | ✅ `Y` | CONFIG_HWSPINLOCK_SPRD |
| 97 | `sprd_iq.ko` | ✅ `KO` | sprd_iq.ko |
| 98 | `sprd_manufacturer_model.ko` | ✅ `KO` | sprd_manufacturer_model.ko |
| 99 | `sprd_mipi.ko` | ✅ `KO` | sprd_mipi.ko |
| 100 | `sprd_modem_loader.ko` | ✅ `Y` | CONFIG_SPRD_MODEM_LOADER |
| 101 | `sprd_pdbg.ko` | ✅ `KO` | sprd_pdbg.ko |
| 102 | `sprd_pmic_syscon.ko` | ✅ `Y` | CONFIG_SPRD_PMIC_SYSCON |
| 103 | `sprd_power_manager.ko` | ✅ `KO` | sprd_power_manager.ko |
| 104 | `sprd_power_stat.ko` | ✅ `KO` | sprd_debugstat.ko |
| 105 | `sprd_sensor.ko` | ✅ `Y` | CONFIG_SPRD_SENSOR |
| 106 | `sprd_shm.ko` | ✅ `KO` | sprd_shm.ko |
| 107 | `sprd_sip_svc.ko` | ✅ `KO` | sprd_sip_svc.ko |
| 108 | `sprd_soc_id.ko` | ✅ `Y` | CONFIG_SPRD_SOCID |
| 109 | `sprd_systimer.ko` | ✅ `KO` | sprd_systimer.ko |
| 110 | `sprd_time_sync.ko` | ✅ `KO` | sprd_time_sync.ko |
| 111 | `sprd_time_sync_cp.ko` | ✅ `KO` | sprd_time_sync_cp.ko |
| 112 | `sprd_wdt_fiq.ko` | ✅ `Y` | CONFIG_SPRD_WATCHDOG_FIQ |
| 113 | `trusty-ipc.ko` | ✅ `KO` | trusty-ipc.ko |
| 114 | `trusty-irq.ko` | ✅ `KO` | trusty-irq.ko |
| 115 | `trusty-log.ko` | ✅ `KO` | trusty-log.ko |
| 116 | `trusty-pm.ko` | ✅ `KO` | trusty-pm.ko |
| 117 | `trusty-tui.ko` | ✅ `KO` | trusty-tui.ko |
| 118 | `trusty-virtio.ko` | ✅ `KO` | trusty-virtio.ko |
| 119 | `trusty.ko` | ✅ `KO` | trusty.ko |
| 120 | `twofish_common.ko` | ✅ `Y` | CONFIG_CRYPTO_TWOFISH_COMMON |
| 121 | `twofish_generic.ko` | ✅ `Y` | CONFIG_CRYPTO_TWOFISH |
| 122 | `ums512-clk.ko` | ✅ `KO` | ums512-clk.ko |
| 123 | `unisoc-iommu.ko` | ✅ `BUILT-IN` | triage + config pendukung =y |
| 124 | `unisoc-mailbox.ko` | ✅ `KO` | unisoc-mailbox.ko |
| 125 | `usb_f_vser.ko` | ✅ `KO` | usb_f_vser.ko |
| 126 | `virt-dma.ko` | ✅ `Y` | CONFIG_DMA_VIRTUAL_CHANNELS |
| 127 | `vsp.ko` | ✅ `Y` | CONFIG_SPRD_VSP |
| 128 | `zram.ko` | ✅ `Y` | CONFIG_ZRAM |
| 129 | `zsmalloc.ko` | ✅ `Y` | CONFIG_ZSMALLOC |
