/* SPDX-License-Identifier: GPL-2.0
 *
 * asoc-618-compat.h — shim API ASoC legacy 5.4 -> component API 6.18.
 *
 * Latar: fork GPL Samsung T618 (sound/soc/sprd) ditulis untuk ASoC 5.4.
 * API legacy snd_soc_codec/snd_soc_platform dihapus mainline sejak 4.17,
 * digantikan snd_soc_component. Port ini meminimalkan diff sumber fork
 * dengan shim tipe + fungsi, sehingga .c fork hampir tak tersentuh.
 *
 * Pemetaan (terverifikasi vs ktree618/include/sound/soc*.h):
 *   snd_soc_codec        -> snd_soc_component
 *   snd_soc_platform     -> snd_soc_component
 *   snd_soc_codec_driver -> snd_soc_component_driver  (+ read/write/... langsung;
 *                            nested .component_driver dibuka lewat sed)
 *   snd_soc_platform_driver -> snd_soc_component_driver
 *   snd_soc_register_codec(dev, drv, dai, n)  -> devm_snd_soc_register_component()
 *   snd_soc_register_platform(dev, drv)       -> devm_snd_soc_register_component()
 *   snd_soc_codec_get_drvdata(c)              -> snd_soc_component_get_drvdata(c)
 *   snd_soc_kcontrol_codec(k)                 -> snd_soc_kcontrol_component(k)
 *   snd_soc_dapm_to_codec(d)                  -> snd_soc_dapm_to_component(d)
 *   snd_soc_codec_get_dapm(c)                 -> snd_soc_component_get_dapm(c)
 *   snd_soc_platform_get_drvdata(p)           -> snd_soc_component_get_drvdata(p)
 *   snd_soc_platform_set_drvdata(p, d)        -> snd_soc_platform_set_drvdata shim
 *   .aif_name (snd_soc_pcm_stream)            -> vxtux_aif_name() (asoc-618-aif.h)
 *
 * CATATAN: shim ini HANYA untuk berkas fork di drivers/vxtux/sound/asoc.
 * Jangan di-include dari kode lain.
 */
#ifndef _VXTUX_ASOC_618_COMPAT_H
#define _VXTUX_ASOC_618_COMPAT_H

#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>	/* of_find_device_by_node() (6.18: pindah dari of.h) */
#include <linux/vmalloc.h>
#include <sound/soc.h>
#include <sound/soc-component.h>
#include <sound/soc-dai.h>
#include <sound/soc-dapm.h>

/* aif_name (dihapus dari snd_soc_pcm_stream 6.18) dipulihkan lewat tabel
 * dari fork — lihat asoc-618-aif.h (dibuat unisoc/work/gen_aif_table.sh). */
#include "asoc-618-aif.h"

/* ---- tipe ---- */
#define snd_soc_codec		snd_soc_component
#define snd_soc_platform	snd_soc_component
#define snd_soc_codec_driver	snd_soc_component_driver
#define snd_soc_platform_driver snd_soc_component_driver

/* ---- fungsi helper legacy ---- */
#define snd_soc_codec_get_drvdata(c)	snd_soc_component_get_drvdata(c)
#define snd_soc_kcontrol_codec(k)	snd_soc_kcontrol_component(k)
#define snd_soc_dapm_to_codec(d)	snd_soc_dapm_to_component(d)
#define snd_soc_codec_get_dapm(c)	snd_soc_component_get_dapm(c)

#define snd_soc_platform_get_drvdata(p)	snd_soc_component_get_drvdata(p)
/* Signature legacy: (platform, data). Karena tipe platform = component
 * lewat rename, teruskan langsung. */
#define snd_soc_platform_set_drvdata(p, d)	snd_soc_component_set_drvdata(p, d)

/* ---- registrasi (5.4 register_* -> 6.18 devm component) ---- */
#define snd_soc_register_codec(dev, drv, dai, ndai) \
	devm_snd_soc_register_component(dev, drv, dai, ndai)
#define snd_soc_unregister_codec(dev)			do { } while (0)
#define snd_soc_register_platform(dev, drv) \
	devm_snd_soc_register_component(dev, drv, NULL, 0)
#define snd_soc_unregister_platform(dev)		do { } while (0)

/* ---- akses register codec: 5.4 snd_soc_{read,write,update_bits} tidak ada
 * lagi sebagai fungsi; semuanya lewat snd_soc_component_* (6.18). ---- */
#define snd_soc_read(c, r)		snd_soc_component_read((c), (r))
#define snd_soc_write(c, r, v)		snd_soc_component_write((c), (r), (v))
#define snd_soc_update_bits(c, r, m, v)	snd_soc_component_update_bits((c), (r), (m), (v))

/* ---- ops yang pindah nama/struct di 6.18 ---- */
/* .pcm_new (platform) -> component .pcm_construct. Signature persis sama
 * (struct snd_soc_pcm_runtime *), jadi rename murni. Buffer DMA pakai
 * managed API (snd_pcm_set_managed_buffer_all) sehingga .pcm_free dihapus
 * (fungsi lama diberi __maybe_unused oleh sed, tak dipanggil lagi). */
#define pcm_new		pcm_construct

/* 5.4 .compr_ops di platform driver -> 6.18 hanya via dai_link (compress path
 * mainline). Fork sprd-compr-2stage-dma.ko memakai jalur itu, jadi portnya
 * DITUNDA (lihat docs/status): bukan dikonversi asal-asalan. */

/* ---- nama bit clock/frame provider (5.4 CBS/CBM -> 6.18 CBC/CBP) ----
 * Commit mainline "ASoC: soc-dai.h: rename CBS/CBM to CBC/CBP" mengganti
 * penamaan "master/slave" menjadi "provider/consumer". Nilai bit identik
 * per pasangan: CBS_CFS=CBC_CFC, CBS_CFM=CBC_CFP, CBM_CFS=CBP_CFC,
 * CBM_CFM=CBP_CFP (terverifikasi di include/sound/soc-dai.h 6.18). */
#ifndef SND_SOC_DAIFMT_CBS_CFS
#define SND_SOC_DAIFMT_CBS_CFS	SND_SOC_DAIFMT_CBC_CFC
#define SND_SOC_DAIFMT_CBS_CFM	SND_SOC_DAIFMT_CBC_CFP
#define SND_SOC_DAIFMT_CBM_CFS	SND_SOC_DAIFMT_CBP_CFC
#define SND_SOC_DAIFMT_CBM_CFM	SND_SOC_DAIFMT_CBP_CFP
#endif

/* ---- DPCM trigger: 6.18 hanya PRE/POST (BESPOKE dihapus) ----
 * SND_SOC_DPCM_TRIGGER_BESPOKE dipetakan ke POST supaya kode fork tetap
 * validasi rentang 0..BESPOKE; nilai DT 2 ke atas kini ditolak (0 dipakai),
 * bukan diteruskan ke enum yang tak ada anggotanya. */
#ifndef SND_SOC_DPCM_TRIGGER_BESPOKE
#define SND_SOC_DPCM_TRIGGER_BESPOKE	SND_SOC_DPCM_TRIGGER_POST
#endif

/* ---- GPIO legacy (5.4 GPIOF_* -> 6.18 hanya GPIOF_IN/OUT_INIT_*) ----
 * 6.18 menyisakan GPIOF_IN, GPIOF_OUT_INIT_LOW/HIGH di include/linux/gpio.h.
 * GPIOF_DIR_OUT/INIT_* dipetakan ke bit yang sama (DIR_OUT = 0, INIT_HIGH =
 * BIT(1)) sehingga GPIOF_DIR_OUT|GPIOF_INIT_HIGH == GPIOF_OUT_INIT_HIGH. */
#ifndef GPIOF_DIR_OUT
#define GPIOF_DIR_OUT		0
#define GPIOF_DIR_IN		GPIOF_IN
#define GPIOF_INIT_HIGH		(1 << 1)
#define GPIOF_INIT_LOW		(0 << 1)
#endif

/* of_get_named_gpio_flags() dihapus di 6.18; flags out-param (selalu NULL di
 * kode fork) dibuang, tinggal nomor GPIO. */
#define of_get_named_gpio_flags(np, prop, idx, flags)	\
	of_get_named_gpio((np), (prop), (idx))

#endif /* _VXTUX_ASOC_618_COMPAT_H */
