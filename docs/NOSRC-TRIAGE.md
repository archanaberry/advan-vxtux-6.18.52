# Triage blob NOSRC — mana yang benar-benar hilang

Dibuat oleh `donors/nosrc_triage.py` dari blob stok itu sendiri (`nm` + `strings`) dan dari isi pohon 6.18.52-VXTux-UMS512. Bukan tebakan.

PORT-AUDIT menyebut blob **NOSRC** kalau tidak ada Makefile yang memetakan simbol CONFIG ke modul bernama itu. Itu **bukan** bukti sumbernya tidak ada. Kolom di bawah memisahkan tiga kasus:

- **BUILT-IN** — sumbernya ada dan simbolnya `=y` di `config.final`, jadi kodenya ikut ke `Image` (tidak muncul sebagai `.ko`).
- **MODUL** — sumber ada, simbol `=m`, harusnya jadi `.ko`.
- **SUPERSEDED** — tidak ada sumber dengan nama itu, tapi ≥60% simbol yang di-EXPORT blob sudah ada di build (fungsinya ada di modul lain).
- **SIAP-COBA** — file sumbernya ada *dan* tercantum di Makefile, tetapi simbol CONFIG-nya belum dinyalakan.
- **PERLU-WIRING** — sumbernya ada tapi tidak tercantum di Makefile/Kconfig direktori itu.
- **PERLU-PORT** — sumbernya memang tidak ada di pohon; harus port dari sumber vendor 5.4 atau ditulis ulang.

`overlap` = persen simbol yang di-EXPORT blob itu sudah ada di kumpulan simbol build (`donors/all_syms.txt`).

## Ringkasan

| Verdict | Jumlah |
|---|---:|
| BUILT-IN | 12 |
| SUPERSEDED | 2 |
| SIAP-COBA | 5 |
| PERLU-PORT | 12 |
| BLOB-HILANG | 1 |
| **total** | **32** |

## Tabel

| # | blob | bytes | desc | globals | exports | overlap | sumber di pohon | simbol | config | verdict |
|---:|---|---:|---|---:|---:|---:|---|---|---|---|
| 1 | `sipc-core.ko` | 230910 | SIPC/SBLOCK driver | 154 | 71 | 52 (73%) | `drivers/soc/sprd/sipc/sipc.c` | `CONFIG_SPRD_SIPC` | y | BUILT-IN |
| 2 | `hxchipset_83102e.ko` | 335310 | Himax_common driver | 297 | 40 | 38 (95%) | `drivers/vxtux/touch/hx83102e/hxchipset/himax_ic_HX83102.c` | `obj-y` | y | BUILT-IN |
| 3 | `unisoc-iommu.ko` | 122990 | Unisoc IOMMU Driver | 140 | 8 | 6 (75%) | `drivers/iommu/sprd-iommu.c` | `CONFIG_SPRD_IOMMU` | y | BUILT-IN |
| 4 | `i2c-sprd-hw.ko` | 33806 | Spreadtrum hardware I2C support | 5 | 0 | 0 (—) | `drivers/i2c/busses/i2c-sprd.c` | `CONFIG_I2C_SPRD` | y | BUILT-IN |
| 5 | `ion_cma_heap.ko` | 34502 | - | 7 | 0 | 0 (—) | `drivers/dma-buf/heaps/cma_heap.c` | `CONFIG_DMABUF_HEAPS_CMA` | y | BUILT-IN |
| 6 | `misc_sprd_uid.ko` | 17990 | Spreadtrum uid driver | 5 | 0 | 0 (—) | `drivers/soc/sprd/soc_id/sprd_uid.c` | `CONFIG_SPRD_UID` | y | BUILT-IN |
| 7 | `musb_sprd.ko` | 65934 | Inventra Dual-Role USB Controller Driver, v6.0 | 10 | 0 | 0 (—) | `drivers/usb/musb/musb_sprd_vxtux.c` | `CONFIG_USB_MUSB_SPRD_VXTUX` | y | BUILT-IN |
| 8 | `pinctrl-sprd-sharkl5Pro.ko` | 49870 | SPREADTRUM Pin Controller Driver | 6 | 0 | 0 (—) | `drivers/pinctrl/sprd/pinctrl-sprd-sharkl5pro.c` | `CONFIG_PINCTRL_SPRD_SHARKL5PRO` | y | BUILT-IN |
| 9 | `sblock_bridge.ko` | 21150 | SIPC/sblock_bridge driver | 5 | 0 | 0 (—) | `drivers/soc/sprd/sipc/sblock.c` | `CONFIG_SPRD_SIPC` | y | BUILT-IN |
| 10 | `sbuf_bridge.ko` | 22334 | SIPC/sbuf_bridge driver | 5 | 0 | 0 (—) | `drivers/soc/sprd/sipc/sbuf.c` | `CONFIG_SPRD_SIPC` | y | BUILT-IN |
| 11 | `snd-soc-sprd-dummy-codec.ko` | 16806 | Dummy Playback ALSA SoC Codec Driver | 8 | 0 | 0 (—) | `drivers/vxtux/sound/asoc/codec/dummy-codec.c` | `CONFIG_VXTUX_SND_SPRD_DUMMY_CODEC` | y | BUILT-IN |
| 12 | `sprd_camera.ko` | 2650982 | SPRD CAM Driver | 500 | 9 | 0 (0%) | `drivers/media/platform/sprd/pipeline/sprd_dcam.c` | `CONFIG_MEDIA_SPRD_PIPELINE` | y | BUILT-IN |
| 13 | `snd-soc-sprd-card.ko` | 131502 | ASoC SPRD Sound Card Utils - Hooks | 90 | 33 | 33 (100%) | `—` | `—` | — | SUPERSEDED |
| 14 | `sha1-ce.ko` | 19558 | SHA1 secure hash using ARMv8 Crypto Extensions | 10 | 0 | 0 (—) | `lib/crypto/arm64/sha1-ce-core.S` | `—` | — | SUPERSEDED |
| 15 | `jpg.ko` | 82918 | SPRD JPG Driver | 15 | 0 | 0 (—) | `drivers/misc/sprd_jpg/sprd_jpg_main.c` | `—` | absent | SIAP-COBA |
| 16 | `snd-soc-sprd-vbc-fe.ko` | 49134 | SPRD ASoC FRONT END CPU DAI | 5 | 0 | 0 (—) | `drivers/vxtux/sound/asoc/dai_v4/vbc-dai.c` | `—` | absent | SIAP-COBA |
| 17 | `sprd-compr-2stage-dma.ko` | 85462 | ASoC Spreadtrum Compress Platform driver | 13 | 0 | 0 (—) | `sound/soc/sprd/sprd-pcm-compress.c` | `—` | absent | SIAP-COBA |
| 18 | `sprd_cpp.ko` | 192670 | CPP Driver | 56 | 0 | 0 (—) | `drivers/media/platform/sprd/cpp/cpp_core.c` | `—` | absent | SIAP-COBA |
| 19 | `vsp.ko` | 115318 | - | 19 | 0 | 0 (—) | `drivers/media/platform/sprd/vsp/sprd_vsp_main.c` | `—` | absent | SIAP-COBA |
| 20 | `bq2560x-charger.ko` | 60766 | BQ2560X Charger Driver | 6 | 0 | 0 (—) | `—` | `—` | — | PERLU-PORT |
| 21 | `mmc_swcq.ko` | 72670 | MMC Host Software Queue support | 24 | 5 | 0 (0%) | `—` | `—` | — | PERLU-PORT |
| 22 | `snd-soc-sprd-codec-sc2730-power-dev.ko` | 29886 | SPRD audio power regulator device | 4 | 0 | 0 (—) | `—` | `—` | — | PERLU-PORT |
| 23 | `snd-soc-sprd-codec-sc2730-power.ko` | 57542 | SPRD audio power regulator driver | 7 | 0 | 0 (—) | `—` | `—` | — | PERLU-PORT |
| 24 | `sprd-cpufreq-public.ko` | 17334 | - | 8 | 1 | 0 (0%) | `—` | `—` | — | PERLU-PORT |
| 25 | `sprd-cpufreq-v2.ko` | 56030 | sprd cpufreq v2 driver | 10 | 0 | 0 (—) | `—` | `—` | — | PERLU-PORT |
| 26 | `sprd_7sreset.ko` | 23350 | Unisoc sc27xx 7SReset Driver | 6 | 0 | 0 (—) | `—` | `—` | — | PERLU-PORT |
| 27 | `sprd_iq.ko` | 32566 | SPRD IQ driver | 6 | 0 | 0 (—) | `—` | `—` | — | PERLU-PORT |
| 28 | `sprd_manufacturer_model.ko` | 16902 | - | 5 | 0 | 0 (—) | `—` | `—` | — | PERLU-PORT |
| 29 | `sprd_pdbg.ko` | 40854 | sprd power debug driver | 8 | 1 | 0 (0%) | `—` | `—` | — | PERLU-PORT |
| 30 | `sprd_power_stat.ko` | 42670 | sprd debug stat driver | 15 | 2 | 0 (0%) | `—` | `—` | — | PERLU-PORT |
| 31 | `sprd_sip_svc.ko` | 30326 | sprd implementation-specific services | 6 | 1 | 0 (0%) | `—` | `—` | — | PERLU-PORT |
| 32 | `sprd_vdsp.ko` | 0 | (blob tidak ada) | 0 | 0 | 0 (—) | `—` | `—` | — | BLOB-HILANG |

## Kasus yang sudah tertutup lewat jalur lain

- `sha1-ce.ko` → **SUPERSEDED**. lib/crypto/sha1.c meng-include `$(SRCARCH)/sha1.h`; untuk arm64 `lib/crypto/arm64/sha1.h` memanggil `__sha1_ce_transform` di bawah `kernel_neon_begin()` begitu CPU punya fitur SHA1 (static key `have_ce`). Jadi shash "sha1" bawaan `crypto/sha1.c` SUDAH memakai Crypto Extensions; sha1-ce.ko stok tidak perlu di-port.

## Kenapa blob stok tidak bisa dipakai apa adanya

Semua blob 5.4 di atas membawa vermagic `5.4.210 SMP preempt mod_unload modversions aarch64`, jadi `insmod` ke 6.18.52 **selalu** ditolak kernel. Penggantinya harus dibangun dari sumber — tidak ada jalan pintas memakai blob asli.
