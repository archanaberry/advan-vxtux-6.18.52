# Manifest blob firmware Advan VX Lite T1030 (2026-10-23)

Sumber: `E:ackupan-ori-advantabvxlite_t1.1ackupendor_boot_a.bin`
(partisi `vendor_boot_a`, 104.857.600 B, header `VNDRBOOT` v4, page 4096,
`vendor_ramdisk_size` = 22.791.010 B, lz4 **legacy** `02 21 4c 18`).

Dibongkar dengan `donors/fw_unpack.py` (dekoder LZ4-legacy + cpio newc murni
Python, tanpa `unpack_bootimg`/`lz4`): 22.791.010 B -> 52.246.016 B cpio,
**640 entri, 129 `.ko`** di `lib/modules/`.

Hasil ekstrak lengkap: `donors/stock_vendor_modules_all/` (129 `.ko`, 14 MB).
`donors/stock_vendor_modules/` (31 `.ko`) yang lama **identik byte-per-byte**
dengan firmware (sha256 ke-31 filenya cocok) — jadi tidak ada blob repo yang
korup; yang belum pernah disimpan adalah 98 sisanya.

Padanan di build TICK12 (`make modules`, 227 `.ko`): **55/129** punya
modul dengan nama sama. Kolom `build` = `KO` kalau ada `.ko` hasil build
dengan nama sama (normalisasi `-`/`_`). Nama yang `—` belum punya padanan
nama; lihat `docs/PORT-AUDIT.md` untuk sebabnya (built-in `=y`, simbol
hantu, atau sumber belum ada di pohon).

| # | blob | bytes | sha256 | build |
|---:|---|---:|---|---|
| 1 | `aes-ce-ccm.ko` | 21182 | `f759d14f149b4359a15d17e1724aabc8…` | KO |
| 2 | `aes-neon-blk.ko` | 53990 | `52a56ea61ddeda196625f0bdf802d8eb…` | KO |
| 3 | `agdsp_access.ko` | 50182 | `fa9c2e6d0e6cb14fc8369114dd50045c…` | KO |
| 4 | `apsys-dvfs.ko` | 327046 | `96de6be2ad650800123d68f1a86aa6d9…` | KO |
| 5 | `arc4.ko` | 15830 | `129a3e1ab2aeb04cc09736c72c468b3c…` | — |
| 6 | `asix.ko` | 174718 | `a3e3ccb352021d211a11375a49ec9f73…` | KO |
| 7 | `audio-dsp-dump.ko` | 75998 | `a6ac75cca97cb4ce63f7c822b797e7ec…` | KO |
| 8 | `audio-pipe.ko` | 46542 | `492d3d0af94193f9a6104a69b47d9f1f…` | KO |
| 9 | `audio_mem.ko` | 52374 | `e15fade80ff8086a2bbcc4d32d026ac5…` | — |
| 10 | `audio_sipc.ko` | 93310 | `99ffdab75a205b1f71942d3d3b59f56e…` | KO |
| 11 | `ax88179_178a.ko` | 67086 | `36498134ee6aaa7ae0d73d6dc67faa0b…` | KO |
| 12 | `bq2560x-charger.ko` | 60766 | `5565e24395babe5d1bc31944a2c9a0cc…` | — |
| 13 | `clk-sprd.ko` | 43118 | `fd7496af9696a3ae70e5332c953d0e6c…` | KO |
| 14 | `core.ko` | 21390 | `7e7b7816c97fd64a1014d60023a7327e…` | — |
| 15 | `extcon-usb-gpio.ko` | 20710 | `3723bc974fa9197dc943135e82374e86…` | KO |
| 16 | `ghash-ce.ko` | 39486 | `a3c69adb1a7b994c554a02defd0ab84f…` | — |
| 17 | `gpio-eic-sprd.ko` | 45982 | `9eaaa8956d9282c3e6288f4cf7edf940…` | — |
| 18 | `gpio-pmic-eic-sprd.ko` | 22094 | `72b856e5578c1ba4a2e43aac7cb06c53…` | KO |
| 19 | `gpio-sprd.ko` | 26358 | `953ff933a5a485cc9df4e5deb1edef9d…` | — |
| 20 | `gpio.ko` | 21878 | `a1652e158b964ffe9ad254e39adc45c5…` | — |
| 21 | `hxchipset_83102e.ko` | 335310 | `dad10858e9424db02bffe3ee9ea805a5…` | — |
| 22 | `i2c-sprd-hw.ko` | 33806 | `b1d6b9e854ea7930aa0724b14d90ae45…` | — |
| 23 | `i2c-sprd.ko` | 47462 | `9ecff50d2d19aed6f1e777656fdaa56d…` | — |
| 24 | `ion_cma_heap.ko` | 34502 | `c97a86f16508cb2f9df9a22b25b8345f…` | — |
| 25 | `ion_ipc_trusty.ko` | 24086 | `511e09cf51cea8071df4ea499929b851…` | KO |
| 26 | `jpg.ko` | 82918 | `77f2ed08771f882c5ae5bf7bcb0b9f42…` | — |
| 27 | `kfifo_buf.ko` | 21390 | `797092018be2c83aa866a6b4c654f99a…` | — |
| 28 | `leds-sc27xx-bltc.ko` | 20198 | `10de1f2b4ade79c7febc445e4e3e4607…` | — |
| 29 | `ledtrig-pattern.ko` | 22230 | `4223585a9f4cb65c14fec5791fe61bce…` | — |
| 30 | `mcdt_hw.ko` | 52902 | `710d2141fc5ced4455e22628bec8222b…` | — |
| 31 | `mipi_driver.ko` | 66870 | `5a8def7182e0c2157dff417824113ef1…` | KO |
| 32 | `misc_sprd_uid.ko` | 17990 | `7603180635cb9413859a61eb485797ef…` | — |
| 33 | `mmc_hsq.ko` | 21790 | `53cbed48adcac6007a2cdc4404615c9d…` | KO |
| 34 | `mmc_swcq.ko` | 72670 | `e8588dcd2b881c5fa3e1ed7bb26ebc9b…` | — |
| 35 | `mmdvfs.ko` | 376054 | `53f361ee21a9bb1124b667229991d143…` | KO |
| 36 | `musb_hdrc.ko` | 345806 | `c96afa202e674ac1cb899d3395df6608…` | — |
| 37 | `musb_sprd.ko` | 65934 | `35d2940332f39eb9a6fb3998f6768211…` | — |
| 38 | `novatek_ts_spi.ko` | 341614 | `9e894e2e22a7b2a362251283bea54502…` | KO |
| 39 | `nvmem-sc27xx-efuse.ko` | 21622 | `438cf03658cfc2dfd8a299e432399659…` | KO |
| 40 | `nvmem_sprd_cache_efuse.ko` | 19478 | `88f483e5a016d1d53d06789cb0942bf9…` | KO |
| 41 | `nvmem_sprd_efuse.ko` | 22574 | `d87d11e7abefaeac436430734cc7bc7d…` | KO |
| 42 | `phy-sprd-sharkl5Pro.ko` | 29038 | `ad7210199123bd13c243a97984f00cf7…` | — |
| 43 | `pinctrl-sprd-sharkl5Pro.ko` | 49870 | `f0a524df84327e8507a60764e7e84399…` | — |
| 44 | `pinctrl-sprd.ko` | 31830 | `d1252ba5b1c76724da86def43b73eb08…` | — |
| 45 | `pinctrl.ko` | 20542 | `0dcd0ebc638c32ba630c9ede7748c4fd…` | — |
| 46 | `pwm-sprd.ko` | 19494 | `d073d498dd64015b8ca91cbc1380aa1d…` | — |
| 47 | `pwm_bl.ko` | 29078 | `eedc3d6e7efb5a82891938bb49c130e0…` | KO |
| 48 | `rtc-sc27xx.ko` | 25830 | `216eb7a06d09a1d1d9ef47fcae1cee04…` | KO |
| 49 | `sblock_bridge.ko` | 21150 | `75193cd96ceb585ce6e39924090a175d…` | — |
| 50 | `sbuf_bridge.ko` | 22334 | `1859f9fe1e234647c979e7ee26963968…` | — |
| 51 | `sc2730-regulator.ko` | 34518 | `e3614d2a81083a9ae1368030d2e4c29c…` | KO |
| 52 | `sc27xx-poweroff.ko` | 19558 | `4284cba2f7ca493e091a16731fe644e8…` | — |
| 53 | `sc27xx-vibra.ko` | 19574 | `fc5016303ab73a403434bf76d8fa35aa…` | — |
| 54 | `sc27xx_adc.ko` | 42742 | `e943fe3463c425cb596220433227b5ac…` | KO |
| 55 | `sc27xx_fuel_gauge.ko` | 114374 | `d1f5514d59d581a526fbfc31aa1ed9de…` | — |
| 56 | `sc27xx_typec.ko` | 24342 | `45f5f4efe727d0fc01fd788ccc61886f…` | KO |
| 57 | `sdhci-sprd.ko` | 145726 | `6d11b178e6d39a94ce8cd3acf7499fc5…` | KO |
| 58 | `sensorhub.ko` | 69982 | `e7a6663c7de3d08f0c6e63f58fa60eb4…` | KO |
| 59 | `sha1-ce.ko` | 19558 | `b6a539c3de7e84d527e9a76f7d21787d…` | — |
| 60 | `sipc-core.ko` | 230910 | `1ed46954bda6f687bc01a5e046521328…` | — |
| 61 | `sipx.ko` | 62510 | `08ca0c6c541dd1c80e23ce797921a1b3…` | — |
| 62 | `slog_bridge.ko` | 34230 | `c776bbcfa55be3770461544ca02d31b1…` | KO |
| 63 | `snd-soc-sprd-card.ko` | 131502 | `2ae4389afd1153412969044e092c7d70…` | — |
| 64 | `snd-soc-sprd-codec-sc2730-power-dev.ko` | 29886 | `5fd9a91886e356139133a84a1b50b414…` | — |
| 65 | `snd-soc-sprd-codec-sc2730-power.ko` | 57542 | `e440c4b138d3845313e0be88835f605a…` | — |
| 66 | `snd-soc-sprd-codec-sc2730.ko` | 416910 | `42dd2e3a1364b446d2dbb126ca1d3376…` | — |
| 67 | `snd-soc-sprd-dummy-codec.ko` | 16806 | `8fc3192a30cc807ee2b545f60a9f3054…` | — |
| 68 | `snd-soc-sprd-vbc-fe.ko` | 49134 | `1c97fa70e3ba28b52a0abad4a245444e…` | — |
| 69 | `snd-soc-sprd-vbc-v4.ko` | 900878 | `54f912220a978a32fe32eff83b39229a…` | KO |
| 70 | `spi-sprd-adi.ko` | 26286 | `b90908c54c18c2a97b8a1dfb3a288ac9…` | — |
| 71 | `spi-sprd.ko` | 40758 | `9dc844a882b68b04e7bca5454afbfdb8…` | KO |
| 72 | `spipe.ko` | 26742 | `bd2d9d47e80665b9d6f47185bc3ebe72…` | — |
| 73 | `spool.ko` | 34038 | `34b00f781608fead4aeaecea7982f64c…` | — |
| 74 | `sprd-charger-manager.ko` | 248326 | `7510d2c4865be1ff6b949068dbe57b65…` | — |
| 75 | `sprd-compr-2stage-dma.ko` | 85462 | `87c81a3effda184dd82ddacecb3efe61…` | — |
| 76 | `sprd-cpufreq-public.ko` | 17334 | `18b0c13e37cc0088188656c6dcc0d6ea…` | — |
| 77 | `sprd-cpufreq-v2.ko` | 56030 | `0144d403cad2fd610ca55ce180b08984…` | — |
| 78 | `sprd-dma.ko` | 39286 | `cb6e572136b0d4079710b01fb57a39c6…` | — |
| 79 | `sprd-dmaengine-pcm.ko` | 61886 | `c2263b616c0c8c8ba55142026203af95…` | KO |
| 80 | `sprd-drm.ko` | 936742 | `9136e8abc002ae0c3146188d85ebb5bf…` | KO |
| 81 | `sprd-gsp.ko` | 615046 | `56c1bd0c4057e3fbd542b96c29c6114b…` | KO |
| 82 | `sprd-ion.ko` | 38406 | `4a691e332ed2e268b5c69b5b262bd7ff…` | — |
| 83 | `sprd-platform-pcm-routing.ko` | 150286 | `d304bf7287134d46926dbac4b77fe1c4…` | — |
| 84 | `sprd-sc27xx-spi.ko` | 21502 | `137ea704798745302696f4361b230f54…` | — |
| 85 | `sprd-sipc-virt-bus.ko` | 15654 | `129448156b0a53f7c77175fc595a45a9…` | KO |
| 86 | `sprd-vsp-pw-domain.ko` | 21678 | `d12391d69698672410df4278db6f2ca3…` | — |
| 87 | `sprd_7sreset.ko` | 23350 | `3ffe75db219defd9913dbfd1995a3238…` | — |
| 88 | `sprd_apipe.ko` | 44230 | `2448c8d091e03b3237125acf4d3a0114…` | KO |
| 89 | `sprd_audcp_boot.ko` | 21622 | `a22885cd53878ec40c1f30a491facf30…` | KO |
| 90 | `sprd_audcp_dvfs.ko` | 61438 | `ec4c549e34a45837be1f9de259a4d4c0…` | KO |
| 91 | `sprd_battery_info.ko` | 30806 | `7d225c370d945e77b4f5b7892788e3f8…` | KO |
| 92 | `sprd_camera.ko` | 2650982 | `bc775c24135426d8ef53db46e86e482d…` | — |
| 93 | `sprd_camsys_pw_domain.ko` | 114246 | `3f447d7c17266af5a2188af221f80f2b…` | — |
| 94 | `sprd_cpp.ko` | 192670 | `2cfcabbf8c8ef7d6d0ccc431b3ee9064…` | — |
| 95 | `sprd_flash_drv.ko` | 24814 | `b08c7121578aa3a635ca3569debf1654…` | KO |
| 96 | `sprd_hwspinlock.ko` | 19118 | `e5bb5b612d315588d4eb5fbc520cb932…` | — |
| 97 | `sprd_iq.ko` | 32566 | `3daf278ba000b56072d3fe8307f75c73…` | — |
| 98 | `sprd_manufacturer_model.ko` | 16902 | `906ffd78742d769a7d511520e53f9ae9…` | — |
| 99 | `sprd_mipi.ko` | 28142 | `ad2485adce24fafdcc6fe680661d8d6e…` | KO |
| 100 | `sprd_modem_loader.ko` | 60742 | `0023a136dac0eff7bf6ad3e9ec7ab4b3…` | — |
| 101 | `sprd_pdbg.ko` | 40854 | `98e0e4795530a026243608356efe2dc7…` | — |
| 102 | `sprd_pmic_syscon.ko` | 20870 | `1a4ce357e74ae4524010c8c0a1c85500…` | — |
| 103 | `sprd_power_manager.ko` | 51406 | `ab652b684f45cc8c9394015a07895220…` | KO |
| 104 | `sprd_power_stat.ko` | 42670 | `44029c9b97530ba7f902a65b5bba83b0…` | — |
| 105 | `sprd_sensor.ko` | 203230 | `2eadc441d4df540b5ba037596e3e92e6…` | — |
| 106 | `sprd_shm.ko` | 29318 | `ea1fc0e8aa123a2afc418820ec3da7fc…` | KO |
| 107 | `sprd_sip_svc.ko` | 30326 | `a3cbf2b01af54298a574e105bb053da3…` | — |
| 108 | `sprd_soc_id.ko` | 20518 | `f181e7d2729910cd970068720fb96a4e…` | — |
| 109 | `sprd_systimer.ko` | 21430 | `bd1039fb261bd86c8a278b4294383d5d…` | KO |
| 110 | `sprd_time_sync.ko` | 27942 | `2a52971ca23a878ce5ffa49ca7664636…` | KO |
| 111 | `sprd_time_sync_cp.ko` | 12934 | `f9ffe2ecfa03110ee06f73bff48a4723…` | KO |
| 112 | `sprd_wdt_fiq.ko` | 28622 | `9b122a1b8eacba29aec895e07136488b…` | — |
| 113 | `trusty-ipc.ko` | 83134 | `b885241a599b22ff33d201f76bdad37a…` | KO |
| 114 | `trusty-irq.ko` | 42894 | `6193696a83a1f5f39dac19c290f981cd…` | KO |
| 115 | `trusty-log.ko` | 28238 | `1fbd5eab39a10102cc0038101e9742a7…` | KO |
| 116 | `trusty-pm.ko` | 16838 | `70c2cbcde49c5ddb717dc2636c41e196…` | KO |
| 117 | `trusty-tui.ko` | 26734 | `4927606c13cdf593276814802a309b99…` | KO |
| 118 | `trusty-virtio.ko` | 37894 | `3833613adb7a83ccc15d05308b4f4c39…` | KO |
| 119 | `trusty.ko` | 54750 | `94d3439ffe66c603626567db69bc9c88…` | KO |
| 120 | `twofish_common.ko` | 23518 | `350172a622a73607f14c4f5729b89a57…` | — |
| 121 | `twofish_generic.ko` | 19270 | `ef19fbb85c9c0faa91c98471925aa147…` | — |
| 122 | `ums512-clk.ko` | 336534 | `7e023e87e4c75d153487b2d4ec2eb468…` | KO |
| 123 | `unisoc-iommu.ko` | 122990 | `599a999b2e13a389b2709392089c55dd…` | — |
| 124 | `unisoc-mailbox.ko` | 53710 | `2c1bedcf4e5d547ce5bbd1e6815b6f6b…` | KO |
| 125 | `usb_f_vser.ko` | 78302 | `2ad4886acfc24689843f55701a43da62…` | KO |
| 126 | `virt-dma.ko` | 19622 | `9d63ae73ae5366f73d1502696869c4f9…` | — |
| 127 | `vsp.ko` | 115318 | `970d36656ded7663d879cc8b2df27e64…` | — |
| 128 | `zram.ko` | 97214 | `932c8b0ed819a46458ae65af67558aa9…` | — |
| 129 | `zsmalloc.ko` | 60886 | `1cbbc4b71c2a8665b7a41e7647e7081d…` | — |
