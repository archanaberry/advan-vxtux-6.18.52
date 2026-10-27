# Keterjangkauan Simbol — 129 blob stok 5.4.210 vs build 6.18.52

Dihasilkan [symreach.py](../donors/symreach.py). Sumber: [symvers_tick14.txt](../donors/symvers_tick14.txt) (15,242 ekspor dari vmlinux + 163 modul), `stock_vendor_modules_all/*.ko` (129 blob, vermagic `5.4.210 SMP preempt mod_unload modversions aarch64`), dan [vmlinux_o_syms.txt](../donors/vmlinux_o_syms.txt) (140,469 simbol dari `nm --defined-only vmlinux.o`, 37,029 di antaranya global; 14,309 dari 14,318 ekspor vmlinux kita ada di situ).

Angka = **referensi** (`SHN_UNDEF` di `.symtab`), bukan simbol unik: satu simbol yang dipakai 60 blob dihitung 60 kali. Simbol unik: 1,488.

## Ringkasan

| Kelas | Referensi | Arti |
|---|---:|---|
| `ok` | **4,750** (85.2%) | diekspor build kita |
| `ekspor` | 0 | ada di vmlinux.o, tinggal `EXPORT_SYMBOL` |
| `rename` | 219 | padanan 6.18 ada, source menyesuaikan |
| `makro` | 330 | definisinya ADA di pohon 6.18 |
| `port` | 24 | hanya disediakan blob stok lain |
| `instr` | 169 | runtime Clang CFI/UBSan di build stok |
| `hilang` | 80 | tidak ada di mana pun, harus ditulis |
| **Total** | **5,572** | |
| CRC modversion beda | 3,334 | dari 4,750 referensi `ok` |
| Blob tanpa port | **91/129** | tidak menunggu modul stok |
| Blob tanpa undefined (export-only) | 0 | |

> **Catatan config.** 146 referensi di kelas `makro` di atas adalah `cpu_hwcaps`, `cpu_hwcap_keys`, `arm64_const_caps_ready`, `kimage_vaddr`, `cpu_number`. Penyedianya sudah ada di pohon (`drivers/misc/sprd/vendor_abi/vendor_abi_arm64.c`), tetapi `CONFIG_VXTUX_VENDOR_ABI` di `config_tick13` masih `# CONFIG_VXTUX_VENDOR_ABI is not set`. Jadi untuk lima simbol ini yang kurang bukan kode dan bukan API: satu config.

## Blob yang tidak butuh port

| blob | undef | ok | ekspor | rename | makro | instr |
|---|---:|---:|---:|---:|---:|---:|
| `agdsp_access.ko` | 33 | 30 | 0 | 2 | 0 | 1 |
| `arc4.ko` | 9 | 8 | 0 | 0 | 0 | 1 |
| `audio-dsp-dump.ko` | 66 | 58 | 0 | 5 | 1 | 2 |
| `audio-pipe.ko` | 32 | 28 | 0 | 2 | 1 | 1 |
| `audio_mem.ko` | 35 | 28 | 0 | 3 | 3 | 1 |
| `audio_sipc.ko` | 45 | 37 | 0 | 3 | 3 | 2 |
| `bq2560x-charger.ko` | 65 | 59 | 0 | 1 | 3 | 2 |
| `clk-sprd.ko` | 28 | 25 | 0 | 2 | 0 | 1 |
| `core.ko` | 20 | 13 | 0 | 3 | 3 | 1 |
| `extcon-usb-gpio.ko` | 23 | 22 | 0 | 0 | 1 | 0 |
| `gpio-eic-sprd.ko` | 26 | 20 | 0 | 2 | 2 | 2 |
| `gpio-pmic-eic-sprd.ko` | 24 | 19 | 0 | 1 | 3 | 1 |
| `gpio-sprd.ko` | 20 | 15 | 0 | 1 | 2 | 2 |
| `hxchipset_83102e.ko` | 96 | 84 | 0 | 5 | 5 | 2 |
| `i2c-sprd-hw.ko` | 37 | 35 | 0 | 0 | 1 | 1 |
| `ion_cma_heap.ko` | 37 | 30 | 0 | 2 | 4 | 1 |
| `ion_ipc_trusty.ko` | 30 | 24 | 0 | 2 | 3 | 1 |
| `jpg.ko` | 75 | 67 | 0 | 2 | 4 | 2 |
| `kfifo_buf.ko` | 20 | 15 | 0 | 1 | 3 | 1 |
| `leds-sc27xx-bltc.ko` | 17 | 15 | 0 | 0 | 1 | 1 |
| `mcdt_hw.ko` | 22 | 17 | 0 | 1 | 2 | 2 |
| `mipi_driver.ko` | 40 | 36 | 0 | 1 | 2 | 1 |
| `misc_sprd_uid.ko` | 17 | 16 | 0 | 0 | 0 | 1 |
| `mmc_hsq.ko` | 20 | 17 | 0 | 1 | 0 | 2 |
| `mmc_swcq.ko` | 62 | 54 | 0 | 3 | 3 | 2 |
| `nvmem-sc27xx-efuse.ko` | 27 | 24 | 0 | 0 | 2 | 1 |
| `nvmem_sprd_cache_efuse.ko` | 23 | 20 | 0 | 1 | 1 | 1 |
| `nvmem_sprd_efuse.ko` | 25 | 22 | 0 | 0 | 2 | 1 |
| `pinctrl-sprd-sharkl5Pro.ko` | 6 | 6 | 0 | 0 | 0 | 0 |
| `pinctrl-sprd.ko` | 28 | 27 | 0 | 0 | 0 | 1 |
| `pwm-sprd.ko` | 15 | 14 | 0 | 0 | 1 | 0 |
| `pwm_bl.ko` | 33 | 27 | 0 | 3 | 1 | 2 |
| `sblock_bridge.ko` | 24 | 23 | 0 | 0 | 0 | 1 |
| `sc2730-regulator.ko` | 29 | 25 | 0 | 0 | 2 | 2 |
| `sc27xx-poweroff.ko` | 19 | 16 | 0 | 1 | 2 | 0 |
| `sc27xx-vibra.ko` | 16 | 16 | 0 | 0 | 0 | 0 |
| `sc27xx_adc.ko` | 34 | 29 | 0 | 0 | 3 | 2 |
| `sc27xx_fuel_gauge.ko` | 72 | 66 | 0 | 1 | 4 | 1 |
| `sc27xx_typec.ko` | 39 | 38 | 0 | 0 | 0 | 1 |
| `sipc-core.ko` | 92 | 79 | 0 | 3 | 8 | 2 |
| `sipx.ko` | 52 | 42 | 0 | 3 | 5 | 2 |
| `snd-soc-sprd-codec-sc2730-power-dev.ko` | 5 | 4 | 0 | 1 | 0 | 0 |
| `snd-soc-sprd-codec-sc2730-power.ko` | 23 | 19 | 0 | 2 | 0 | 2 |
| `snd-soc-sprd-dummy-codec.ko` | 9 | 7 | 0 | 0 | 1 | 1 |
| `snd-soc-sprd-vbc-fe.ko` | 20 | 15 | 0 | 1 | 3 | 1 |
| `spi-sprd-adi.ko` | 32 | 29 | 0 | 1 | 0 | 2 |
| `spipe.ko` | 33 | 30 | 0 | 2 | 0 | 1 |
| `sprd-charger-manager.ko` | 99 | 86 | 0 | 4 | 7 | 2 |
| `sprd-cpufreq-public.ko` | 11 | 6 | 0 | 1 | 2 | 2 |
| `sprd-dma.ko` | 48 | 43 | 0 | 1 | 3 | 1 |
| `sprd-dmaengine-pcm.ko` | 45 | 37 | 0 | 3 | 3 | 2 |
| `sprd-gsp.ko` | 128 | 116 | 0 | 4 | 6 | 2 |
| `sprd-ion.ko` | 41 | 34 | 0 | 3 | 2 | 2 |
| `sprd-platform-pcm-routing.ko` | 9 | 8 | 0 | 0 | 1 | 0 |
| `sprd-sc27xx-spi.ko` | 19 | 15 | 0 | 0 | 3 | 1 |
| `sprd-sipc-virt-bus.ko` | 6 | 6 | 0 | 0 | 0 | 0 |
| `sprd-vsp-pw-domain.ko` | 23 | 21 | 0 | 1 | 0 | 1 |
| `sprd_7sreset.ko` | 16 | 15 | 0 | 0 | 0 | 1 |
| `sprd_apipe.ko` | 34 | 30 | 0 | 1 | 1 | 2 |
| `sprd_audcp_boot.ko` | 22 | 20 | 0 | 1 | 0 | 1 |
| `sprd_audcp_dvfs.ko` | 18 | 17 | 0 | 0 | 0 | 1 |
| `sprd_battery_info.ko` | 23 | 20 | 0 | 1 | 1 | 1 |
| `sprd_camsys_pw_domain.ko` | 43 | 37 | 0 | 1 | 3 | 2 |
| `sprd_cpp.ko` | 78 | 65 | 0 | 3 | 8 | 2 |
| `sprd_flash_drv.ko` | 16 | 13 | 0 | 1 | 0 | 2 |
| `sprd_hwspinlock.ko` | 18 | 18 | 0 | 0 | 0 | 0 |
| `sprd_iq.ko` | 41 | 36 | 0 | 2 | 1 | 2 |
| `sprd_manufacturer_model.ko` | 7 | 6 | 0 | 1 | 0 | 0 |
| `sprd_mipi.ko` | 17 | 13 | 0 | 2 | 0 | 2 |
| `sprd_pmic_syscon.ko` | 23 | 19 | 0 | 1 | 2 | 1 |
| `sprd_power_manager.ko` | 33 | 23 | 0 | 3 | 6 | 1 |
| `sprd_shm.ko` | 43 | 37 | 0 | 2 | 3 | 1 |
| `sprd_sip_svc.ko` | 5 | 3 | 0 | 1 | 0 | 1 |
| `sprd_soc_id.ko` | 14 | 12 | 0 | 1 | 0 | 1 |
| `sprd_systimer.ko` | 10 | 7 | 0 | 2 | 0 | 1 |
| `sprd_time_sync.ko` | 36 | 31 | 0 | 3 | 0 | 2 |
| `sprd_time_sync_cp.ko` | 2 | 2 | 0 | 0 | 0 | 0 |
| `sprd_wdt_fiq.ko` | 31 | 27 | 0 | 1 | 2 | 1 |
| `trusty-ipc.ko` | 69 | 58 | 0 | 5 | 4 | 2 |
| `trusty-irq.ko` | 44 | 39 | 0 | 1 | 3 | 1 |
| `trusty-log.ko` | 39 | 32 | 0 | 4 | 2 | 1 |
| `trusty-tui.ko` | 20 | 17 | 0 | 2 | 0 | 1 |
| `trusty-virtio.ko` | 41 | 33 | 0 | 4 | 4 | 0 |
| `trusty.ko` | 66 | 55 | 0 | 4 | 5 | 2 |
| `twofish_common.ko` | 1 | 1 | 0 | 0 | 0 | 0 |
| `twofish_generic.ko` | 4 | 4 | 0 | 0 | 0 | 0 |
| `ums512-clk.ko` | 18 | 18 | 0 | 0 | 0 | 0 |
| `unisoc-mailbox.ko` | 49 | 44 | 0 | 1 | 2 | 2 |
| `usb_f_vser.ko` | 52 | 44 | 0 | 4 | 2 | 2 |
| `virt-dma.ko` | 12 | 8 | 0 | 0 | 2 | 2 |
| `vsp.ko` | 80 | 70 | 0 | 2 | 6 | 2 |

## Blob terblokir

| blob | undef | ok | port | hilang | makro | modul stok penyedia |
|---|---:|---:|---:|---:|---:|---|
| `aes-ce-ccm.ko` | 23 | 19 | 0 | 1 | 2 | — |
| `aes-neon-blk.ko` | 27 | 23 | 0 | 1 | 2 | — |
| `apsys-dvfs.ko` | 63 | 55 | 0 | 1 | 3 | — |
| `asix.ko` | 80 | 68 | 0 | 4 | 4 | — |
| `ax88179_178a.ko` | 49 | 45 | 0 | 2 | 0 | — |
| `ghash-ce.ko` | 28 | 24 | 0 | 1 | 2 | — |
| `gpio.ko` | 15 | 10 | 2 | 0 | 0 | `core`×2 |
| `i2c-sprd.ko` | 52 | 47 | 0 | 2 | 0 | — |
| `ledtrig-pattern.ko` | 23 | 18 | 0 | 1 | 1 | — |
| `mmdvfs.ko` | 51 | 41 | 0 | 1 | 5 | — |
| `musb_hdrc.ko` | 132 | 114 | 0 | 2 | 8 | — |
| `musb_sprd.ko` | 94 | 84 | 1 | 0 | 6 | `musb_hdrc`×1 |
| `novatek_ts_spi.ko` | 78 | 66 | 0 | 1 | 6 | — |
| `phy-sprd-sharkl5Pro.ko` | 47 | 42 | 0 | 1 | 2 | — |
| `pinctrl.ko` | 17 | 12 | 2 | 0 | 1 | `core`×2 |
| `rtc-sc27xx.ko` | 28 | 22 | 0 | 1 | 3 | — |
| `sbuf_bridge.ko` | 28 | 25 | 1 | 0 | 1 | `sipc-core`×1 |
| `sdhci-sprd.ko` | 130 | 112 | 5 | 1 | 5 | `mmc_swcq`×5 |
| `sensorhub.ko` | 86 | 72 | 0 | 3 | 5 | — |
| `sha1-ce.ko` | 12 | 9 | 0 | 2 | 1 | — |
| `slog_bridge.ko` | 44 | 34 | 5 | 0 | 3 | `sipc-core`×5 |
| `snd-soc-sprd-card.ko` | 63 | 57 | 0 | 1 | 2 | — |
| `snd-soc-sprd-codec-sc2730.ko` | 134 | 116 | 0 | 3 | 11 | — |
| `snd-soc-sprd-vbc-v4.ko` | 89 | 80 | 0 | 1 | 3 | — |
| `spi-sprd.ko` | 55 | 49 | 0 | 2 | 1 | — |
| `spool.ko` | 49 | 42 | 2 | 0 | 1 | `sipc-core`×2 |
| `sprd-compr-2stage-dma.ko` | 70 | 54 | 0 | 1 | 7 | — |
| `sprd-cpufreq-v2.ko` | 68 | 55 | 1 | 1 | 8 | `sprd_sip_svc`×1 |
| `sprd-drm.ko` | 288 | 261 | 0 | 10 | 9 | — |
| `sprd_camera.ko` | 158 | 131 | 0 | 2 | 19 | — |
| `sprd_modem_loader.ko` | 64 | 52 | 1 | 0 | 7 | `sipc-core`×1 |
| `sprd_pdbg.ko` | 51 | 41 | 1 | 1 | 4 | `sprd_sip_svc`×1 |
| `sprd_power_stat.ko` | 30 | 22 | 1 | 0 | 3 | `sprd_sip_svc`×1 |
| `sprd_sensor.ko` | 79 | 64 | 0 | 2 | 8 | — |
| `trusty-pm.ko` | 10 | 7 | 0 | 2 | 0 | — |
| `unisoc-iommu.ko` | 63 | 47 | 0 | 3 | 8 | — |
| `zram.ko` | 129 | 93 | 2 | 19 | 7 | `zsmalloc`×2 |
| `zsmalloc.ko` | 56 | 41 | 0 | 7 | 3 | — |

## Leverage: satu modul di-port, sekian referensi terbuka

| modul stok | referensi terbuka |
|---|---:|
| `sipc-core` | 9 |
| `mmc_swcq` | 5 |
| `core` | 4 |
| `sprd_sip_svc` | 3 |
| `zsmalloc` | 2 |
| `musb_hdrc` | 1 |

## `ekspor` — simbol GLOBAL ada di vmlinux kita, cuma tidak diekspor

0 simbol. Ini yang paling murah: satu baris `EXPORT_SYMBOL[_GPL]()` di file yang sudah ada di pohon. Simbol yang cuma muncul sebagai salinan **lokal** (`t`) dari `static inline` tidak dihitung di sini — itu masuk `makro`.

| simbol | blob |
|---|---:|

## `rename` — padanan 6.18 ada

Semua padanan di bawah **diverifikasi** ada di `vmlinux.o` kita: aturan mekanis (`_nama`, `nama_noprof`) atau tabel `REMAP` di symreach.py 
      yang hanya dipakai kalau targetnya benar-benar ada.

| nama blob 5.4 | padanan | blob |
|---|---|---:|
| `printk` | `_printk` | 79 |
| `kmem_cache_alloc_trace` | `kmem_cache_alloc_noprof` | 45 |
| `__kmalloc` | `__kmalloc_noprof` | 26 |
| `__class_create` | `class_create` | 13 |
| `alloc_workqueue` | `alloc_workqueue_noprof` | 9 |
| `__memset_io` | `memset_io` | 5 |
| `find_next_bit` | `_find_next_bit` | 5 |
| `del_timer` | `timer_delete` | 5 |
| `vmalloc` | `vmalloc_noprof` | 3 |
| `hrtimer_init` | `hrtimer_setup` | 3 |
| `vzalloc` | `vzalloc_noprof` | 3 |
| `__alloc_pages_nodemask` | `__alloc_pages_noprof` | 3 |
| `gpiochip_find` | `gpio_device_find` | 2 |
| `del_timer_sync` | `timer_delete_sync` | 2 |
| `pwm_apply_state` | `pwm_apply_might_sleep` | 2 |
| `no_llseek` | `noop_llseek` | 2 |
| `dma_request_slave_channel` | `dma_request_chan` | 2 |
| `__class_register` | `class_register` | 2 |
| `alloc_pages_exact` | `alloc_pages_exact_noprof` | 2 |
| `dev_printk` | `_dev_printk` | 1 |
| `pwm_request` | `pwm_get` | 1 |
| `pwm_free` | `pwm_put` | 1 |
| `kmemdup` | `kmemdup_noprof` | 1 |
| `find_next_zero_bit` | `_find_next_zero_bit` | 1 |
| `kmem_cache_alloc` | `kmem_cache_alloc_noprof` | 1 |

## `makro` — definisinya ada di pohon kita

75 simbol. Di blob 5.4 ini fungsi sungguhan; di 6.18 definisinya ada tapi **tidak jadi simbol**: `static inline`, `#define`, atau file-nya tidak ikut di-build karena CONFIG mati. Artinya ini urusan compile/config, **bukan** tulis kode baru. Bukti file:line dari pohon kita:

| simbol | blob | jadi makro di |
|---|---:|---|
| `mutex_lock` | 50 | `drivers/clocksource/timer-nxp-stm.c:80` |
| `__list_add_valid` | 28 | `include/linux/list.h:73` |
| `__list_del_entry_valid` | 25 | `include/linux/list.h:114` |
| `usleep_range` | 21 | `drivers/gpu/drm/i915/gt/selftest_slpc.c:8` |
| `of_parse_phandle` | 19 | `include/linux/of.h:986` |
| `debugfs_create_file` | 11 | `include/linux/debugfs.h:125` |
| `wakeup_source_create` | 10 | `drivers/base/power/wakeup.c:80` |
| `wakeup_source_add` | 10 | `drivers/base/power/wakeup.c:165` |
| `of_get_named_gpio_flags` | 9 | `drivers/vxtux/sound/asoc/asoc-618-compat.h:120` |
| `device_init_wakeup` | 8 | `include/linux/pm_wakeup.h:217` |
| `vabits_actual` | 7 | `arch/arm64/include/asm/memory.h:232` |
| `snd_soc_unregister_component` | 7 | `include/sound/soc.h:447` |
| `sg_next` | 6 | `include/linux/scatterlist.h:107` |
| `strlcpy` | 6 | `tools/include/linux/string.h:27` |
| `seq_puts` | 6 | `include/linux/seq_file.h:122` |
| `wakeup_source_remove` | 6 | `drivers/base/power/wakeup.c:185` |
| `debugfs_remove_recursive` | 5 | `include/linux/debugfs.h:324` |
| `cpumask_next` | 5 | `include/linux/cpumask.h:230` |
| `devm_add_action` | 5 | `include/linux/device/devres.h:153` |
| `wakeup_source_destroy` | 5 | `drivers/base/power/wakeup.c:151` |
| `irq_find_mapping` | 4 | `include/linux/irqdomain.h:484` |
| `of_parse_phandle_with_args` | 4 | `include/linux/of.h:1031` |
| `dev_pm_opp_get_freq` | 3 | `include/linux/pm_opp.h:746` |
| `mutex_trylock` | 3 | `include/linux/mutex.h:218` |
| `devm_gpiochip_add_data` | 3 | `include/linux/gpio/driver.h:611` |
| `dma_direct_sync_single_for_cpu` | 3 | `kernel/dma/direct.h:67` |
| `dma_direct_sync_single_for_device` | 3 | `kernel/dma/direct.h:56` |
| `pfn_valid` | 3 | `arch/arm/include/asm/page.h:179` |
| `__alloc_percpu` | 3 | `include/linux/percpu.h:141` |
| `__get_free_pages` | 3 | `include/linux/gfp.h:362` |
| `scatterwalk_map_and_copy` | 2 | `include/crypto/scatterwalk.h:285` |
| `__ion_device_add_heap` | 2 | `include/linux/ion.h:177` |
| `mutex_lock_interruptible` | 2 | `include/linux/mutex.h:169` |
| `drm_err` | 2 | `drivers/gpu/drm/amd/display/include/logger_types.h:31` |
| `simd_skcipher_create_compat` | 1 | `crypto/simd.c:139` |
| `simd_skcipher_free` | 1 | `crypto/simd.c:200` |
| `kfree_skb` | 1 | `include/linux/skbuff.h:1329` |
| `ion_device_remove_heap` | 1 | `include/linux/ion.h:208` |
| `ion_buffer_prep_noncached` | 1 | `include/linux/ion.h:334` |
| `devres_alloc_node` | 1 | `include/linux/device/devres.h:25` |

## `instr` — runtime Clang, bukan API kernel

| simbol | blob |
|---|---:|
| `__stack_chk_guard` | 107 |
| `__cfi_slowpath` | 62 |

## `hilang` — harus ditulis / diganti kode

61 simbol unik. Kolom kanan = kandidat terdekat di vmlinux 6.18.52.

| simbol | blob | kandidat |
|---|---:|---|
| `kmalloc_order_trace` | 6 | — |
| `__ioremap` | 5 | — |
| `fpsimd_context_busy` | 4 | — |
| `dma_direct_map_page` | 3 | `dma_direct_all_ram_mapped`, `dma_direct_alloc` |
| `dma_direct_unmap_page` | 3 | `dma_direct_all_ram_mapped`, `dma_direct_alloc` |
| `usbnet_get_stats64` | 2 | `usbnet_get_drvinfo`, `usbnet_get_endpoints` |
| `__dev_kfree_skb_any` | 2 | — |
| `devm_gpio_request` | 2 | `devm_gpio_chip_release`, `devm_gpio_release` |
| `usbnet_get_link_ksettings` | 1 | `usbnet_get_drvinfo`, `usbnet_get_endpoints` |
| `usbnet_set_link_ksettings` | 1 | `usbnet_set_link_ksettings_mii`, `usbnet_set_msglevel` |
| `of_dev_put` | 1 | — |
| `__rtc_register_device` | 1 | — |
| `sdhci_pltfm_free` | 1 | `sdhci_pltfm_clk_get_max_clock`, `sdhci_pltfm_drv_exit` |
| `iio_triggered_buffer_postenable` | 1 | `iio_trigger_attach_poll_func`, `iio_trigger_consumer_attr_group` |
| `iio_triggered_buffer_predisable` | 1 | `iio_trigger_attach_poll_func`, `iio_trigger_consumer_attr_group` |
| `__iio_trigger_register` | 1 | `__iio_trigger_alloc` |
| `crypto_sha1_finup` | 1 | `crypto_sha1_digest`, `crypto_sha1_export` |
| `snd_soc_of_parse_daifmt` | 1 | `snd_soc_of_get_dai_link_codecs`, `snd_soc_of_get_dai_link_cpus` |
| `snd_soc_component_read32` | 1 | `snd_soc_compensate_channel_connection_map`, `snd_soc_component_async_complete` |
| `dapm_regulator_event` | 1 | — |
| `cpufreq_generic_attr` | 1 | `cpufreq_generic_frequency_table_verify`, `cpufreq_generic_get` |
| `drm_gem_cma_prime_get_sg_table` | 1 | — |
| `drm_gem_cma_prime_vmap` | 1 | — |
| `drm_gem_cma_prime_vunmap` | 1 | — |
| `drm_gem_cma_prime_mmap` | 1 | — |
| `drm_gem_cma_vm_ops` | 1 | — |
| `drm_gem_object_put_unlocked` | 1 | `drm_gem_object_free`, `drm_gem_object_handle_get` |
| `drm_panel_attach` | 1 | `drm_panel_add`, `drm_panel_add_follower` |
| `drm_panel_detach` | 1 | `drm_panel_add`, `drm_panel_add_follower` |
| `ns_to_timeval` | 1 | `ns_to_timespec64` |
| `pm_get_active_wakeup_sources` | 1 | — |
| `devm_gpio_free` | 1 | `devm_gpio_chip_release`, `devm_gpio_release` |
| `__tracepoint_android_vh_psci_tos_resident_on` | 1 | `__tracepoint_add_device_to_group`, `__tracepoint_alarmtimer_cancel` |
| `__tracepoint_android_vh_psci_cpu_suspend` | 1 | `__tracepoint_add_device_to_group`, `__tracepoint_alarmtimer_cancel` |
| `__tracepoint_android_vh_ion_buffer_release` | 1 | `__tracepoint_add_device_to_group`, `__tracepoint_alarmtimer_cancel` |
| `bdget_disk` | 1 | — |
| `bdput` | 1 | — |
| `fsync_bdev` | 1 | — |
| `blk_cleanup_queue` | 1 | — |
| `blkdev_put` | 1 | `blkdev_put_no_open`, `blkdev_put_whole` |

## Lembar kerja per blob terblokir

**`aes-ce-ccm.ko`** — undef 23, ok 19, port 0, hilang 1

- `fpsimd_context_busy`

**`aes-neon-blk.ko`** — undef 27, ok 23, port 0, hilang 1

- `fpsimd_context_busy`

**`apsys-dvfs.ko`** — undef 63, ok 55, port 0, hilang 1

- `__class_create` → `class_create`
- `printk` → `_printk`
- `__ioremap`

**`asix.ko`** — undef 80, ok 68, port 0, hilang 4

- `__kmalloc` → `__kmalloc_noprof`
- `kmem_cache_alloc_trace` → `kmem_cache_alloc_noprof`
- `__dev_kfree_skb_any`
- `usbnet_get_link_ksettings` → `usbnet_get_drvinfo`
- `usbnet_get_stats64` → `usbnet_get_drvinfo`
- `usbnet_set_link_ksettings` → `usbnet_set_link_ksettings_mii`

**`ax88179_178a.ko`** — undef 49, ok 45, port 0, hilang 2

- `__kmalloc` → `__kmalloc_noprof`
- `__dev_kfree_skb_any`
- `usbnet_get_stats64` → `usbnet_get_drvinfo`

**`ghash-ce.ko`** — undef 28, ok 24, port 0, hilang 1

- `fpsimd_context_busy`

**`gpio.ko`** — undef 15, ok 10, port 2, hilang 0

- `gpiochip_find` → `gpio_device_find`
- `printk` → `_printk`

**`i2c-sprd.ko`** — undef 52, ok 47, port 0, hilang 2

- `__kmalloc` → `__kmalloc_noprof`
- `dma_direct_map_page` → `dma_direct_all_ram_mapped`
- `dma_direct_unmap_page` → `dma_direct_all_ram_mapped`

**`ledtrig-pattern.ko`** — undef 23, ok 18, port 0, hilang 1

- `del_timer_sync` → `timer_delete_sync`
- `kmalloc_order_trace`

**`mmdvfs.ko`** — undef 51, ok 41, port 0, hilang 1

- `kmem_cache_alloc_trace` → `kmem_cache_alloc_noprof`
- `printk` → `_printk`
- `__ioremap`

**`musb_hdrc.ko`** — undef 132, ok 114, port 0, hilang 2

- `__kmalloc` → `__kmalloc_noprof`
- `del_timer` → `timer_delete`
- `dev_printk` → `_dev_printk`
- `find_next_bit` → `_find_next_bit`
- `kmem_cache_alloc_trace` → `kmem_cache_alloc_noprof`
- `printk` → `_printk`
- `dma_direct_map_page` → `dma_direct_all_ram_mapped`
- `dma_direct_unmap_page` → `dma_direct_all_ram_mapped`

**`musb_sprd.ko`** — undef 94, ok 84, port 1, hilang 0

- `__class_create` → `class_create`

**`novatek_ts_spi.ko`** — undef 78, ok 66, port 0, hilang 1

- `__kmalloc` → `__kmalloc_noprof`
- `alloc_workqueue` → `alloc_workqueue_noprof`
- `kmem_cache_alloc_trace` → `kmem_cache_alloc_noprof`
- `printk` → `_printk`
- `kmalloc_order_trace`

**`phy-sprd-sharkl5Pro.ko`** — undef 47, ok 42, port 0, hilang 1

- `printk` → `_printk`
- `of_dev_put`

**`pinctrl.ko`** — undef 17, ok 12, port 2, hilang 0

- `printk` → `_printk`

**`rtc-sc27xx.ko`** — undef 28, ok 22, port 0, hilang 1

- `printk` → `_printk`
- `__rtc_register_device`

**`sbuf_bridge.ko`** — undef 28, ok 25, port 1, hilang 0


**`sdhci-sprd.ko`** — undef 130, ok 112, port 5, hilang 1

- `__kmalloc` → `__kmalloc_noprof`
- `del_timer` → `timer_delete`
- `kmem_cache_alloc_trace` → `kmem_cache_alloc_noprof`
- `no_llseek` → `noop_llseek`
- `printk` → `_printk`
- `sdhci_pltfm_free` → `sdhci_pltfm_clk_get_max_clock`

**`sensorhub.ko`** — undef 86, ok 72, port 0, hilang 3

- `__class_create` → `class_create`
- `alloc_workqueue` → `alloc_workqueue_noprof`
- `find_next_bit` → `_find_next_bit`
- `printk` → `_printk`
- `__iio_trigger_register` → `__iio_trigger_alloc`
- `iio_triggered_buffer_postenable` → `iio_trigger_attach_poll_func`
- `iio_triggered_buffer_predisable` → `iio_trigger_attach_poll_func`

**`sha1-ce.ko`** — undef 12, ok 9, port 0, hilang 2

- `crypto_sha1_finup` → `crypto_sha1_digest`
- `fpsimd_context_busy`

**`slog_bridge.ko`** — undef 44, ok 34, port 5, hilang 0

- `printk` → `_printk`

**`snd-soc-sprd-card.ko`** — undef 63, ok 57, port 0, hilang 1

- `printk` → `_printk`
- `snd_soc_of_parse_daifmt` → `snd_soc_of_get_dai_link_codecs`

**`snd-soc-sprd-codec-sc2730.ko`** — undef 134, ok 116, port 0, hilang 3

- `alloc_workqueue` → `alloc_workqueue_noprof`
- `printk` → `_printk`
- `dapm_regulator_event`
- `devm_gpio_request` → `devm_gpio_chip_release`
- `snd_soc_component_read32` → `snd_soc_compensate_channel_connection_map`

**`snd-soc-sprd-vbc-v4.ko`** — undef 89, ok 80, port 0, hilang 1

- `__class_create` → `class_create`
- `__kmalloc` → `__kmalloc_noprof`
- `printk` → `_printk`
- `vmalloc` → `vmalloc_noprof`
- `kmalloc_order_trace`

**`spi-sprd.ko`** — undef 55, ok 49, port 0, hilang 2

- `__kmalloc` → `__kmalloc_noprof`
- `dma_direct_map_page` → `dma_direct_all_ram_mapped`
- `dma_direct_unmap_page` → `dma_direct_all_ram_mapped`

**`spool.ko`** — undef 49, ok 42, port 2, hilang 0

- `__class_create` → `class_create`
- `kmem_cache_alloc_trace` → `kmem_cache_alloc_noprof`
- `printk` → `_printk`

**`sprd-compr-2stage-dma.ko`** — undef 70, ok 54, port 0, hilang 1

- `__kmalloc` → `__kmalloc_noprof`
- `__memset_io` → `memset_io`
- `alloc_workqueue` → `alloc_workqueue_noprof`
- `dma_request_slave_channel` → `dma_request_chan`
- `kmem_cache_alloc_trace` → `kmem_cache_alloc_noprof`
- `printk` → `_printk`
- `kmalloc_order_trace`

**`sprd-cpufreq-v2.ko`** — undef 68, ok 55, port 1, hilang 1

- `printk` → `_printk`
- `cpufreq_generic_attr` → `cpufreq_generic_frequency_table_verify`

**`sprd-drm.ko`** — undef 288, ok 261, port 0, hilang 10

- `__class_create` → `class_create`
- `__kmalloc` → `__kmalloc_noprof`
- `kmem_cache_alloc_trace` → `kmem_cache_alloc_noprof`
- `no_llseek` → `noop_llseek`
- `printk` → `_printk`
- `pwm_apply_state` → `pwm_apply_might_sleep`
- `__ioremap`
- `drm_gem_cma_prime_get_sg_table`
- `drm_gem_cma_prime_mmap`
- `drm_gem_cma_prime_vmap`
- `drm_gem_cma_prime_vunmap`
- `drm_gem_cma_vm_ops`
- `drm_gem_object_put_unlocked` → `drm_gem_object_free`
- `drm_panel_attach` → `drm_panel_add`

**`sprd_camera.ko`** — undef 158, ok 131, port 0, hilang 2

- `__kmalloc` → `__kmalloc_noprof`
- `del_timer_sync` → `timer_delete_sync`
- `printk` → `_printk`
- `vzalloc` → `vzalloc_noprof`
- `__ioremap`
- `ns_to_timeval` → `ns_to_timespec64`

**`sprd_modem_loader.ko`** — undef 64, ok 52, port 1, hilang 0

- `__class_create` → `class_create`
- `printk` → `_printk`

**`sprd_pdbg.ko`** — undef 51, ok 41, port 1, hilang 1

- `kmem_cache_alloc_trace` → `kmem_cache_alloc_noprof`
- `printk` → `_printk`
- `pm_get_active_wakeup_sources`

**`sprd_power_stat.ko`** — undef 30, ok 22, port 1, hilang 0

- `kmem_cache_alloc_trace` → `kmem_cache_alloc_noprof`
- `printk` → `_printk`

**`sprd_sensor.ko`** — undef 79, ok 64, port 0, hilang 2

- `__kmalloc` → `__kmalloc_noprof`
- `kmem_cache_alloc_trace` → `kmem_cache_alloc_noprof`
- `printk` → `_printk`
- `devm_gpio_free` → `devm_gpio_chip_release`
- `devm_gpio_request` → `devm_gpio_chip_release`

**`trusty-pm.ko`** — undef 10, ok 7, port 0, hilang 2

- `printk` → `_printk`
- `__tracepoint_android_vh_psci_cpu_suspend` → `__tracepoint_add_device_to_group`
- `__tracepoint_android_vh_psci_tos_resident_on` → `__tracepoint_add_device_to_group`

**`unisoc-iommu.ko`** — undef 63, ok 47, port 0, hilang 3

- `__kmalloc` → `__kmalloc_noprof`
- `kmem_cache_alloc_trace` → `kmem_cache_alloc_noprof`
- `printk` → `_printk`
- `__ioremap`
- `__tracepoint_android_vh_ion_buffer_release` → `__tracepoint_add_device_to_group`
- `kmalloc_order_trace`

**`zram.ko`** — undef 129, ok 93, port 2, hilang 19

- `__alloc_pages_nodemask` → `__alloc_pages_noprof`
- `__class_register` → `class_register`
- `find_next_zero_bit` → `_find_next_zero_bit`
- `kmem_cache_alloc_trace` → `kmem_cache_alloc_noprof`
- `printk` → `_printk`
- `vzalloc` → `vzalloc_noprof`
- `bdget_disk`
- `bdgrab`
- `bdput`
- `blk_cleanup_queue`
- `blk_queue_io_min` → `blk_queue_attr_groups`
- `blk_queue_io_opt` → `blk_queue_attr_groups`
- `blk_queue_logical_block_size` → `blk_queue_attr_groups`
- `blk_queue_make_request` → `blk_queue_attr_groups`

**`zsmalloc.ko`** — undef 56, ok 41, port 0, hilang 7

- `__alloc_pages_nodemask` → `__alloc_pages_noprof`
- `kmem_cache_alloc` → `kmem_cache_alloc_noprof`
- `kmem_cache_alloc_trace` → `kmem_cache_alloc_noprof`
- `printk` → `_printk`
- `__ClearPageMovable`
- `__SetPageMovable`
- `__put_page`
- `page_mapping`
- `register_shrinker` → `register_snap_client`
- `unregister_shrinker` → `unregister_asymmetric_key_parser`
- `wait_on_page_bit` → `wait_on_page_writeback`

## Metodologi

1. `.symtab` setiap blob dibaca **pyelftools**: `SHN_UNDEF` = referensi keluar, `__crc_<nama>` = ekspor modul itu.
2. Urutan pencocokan: `Module.symvers` tick13 → `nm vmlinux.o` → padanan `_nama`/`nama_noprof` → `#define`/`static inline` di header (git grep) → ekspor blob stok lain → daftar sanitizer → `hilang`.
3. Seksi `__versions` (64 byte/entri: CRC 8 + nama 56) dibandingkan dengan CRC `Module.symvers`. Beda CRC = `insmod` menolak walau nama ada. Angka besarnya wajar: 5.4.210 → 6.18.52 mengubah banyak tipe.
4. `makro` dipisah dari `hilang` lewat `git grep` definisi di kolom 0 (`^[A-Za-z_].*nama(` atau `#define nama`), karena definisi kernel selalu di kolom 0 sedangkan pemanggilan menjorok. Simbol dengan definisi di pohon berarti masalah compile/config, bukan port kode.
