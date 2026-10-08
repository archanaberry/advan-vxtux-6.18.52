#!/bin/sh
# asoc618_sed.sh — transformasi mekanis 5.4 -> 6.18 untuk port fork sound/soc/sprd.
# Hanya transformasi satu-baris yang aman. Situs multi-baris (register_*,
# component_driver, remove() int->void, codec-driver nested) diperbaiki manual.
# Jalankan:  sh asoc618_sed.sh <dir-berkas-port>
set -u
D=${1:?penggunaan: asoc618_sed.sh <dir-berkas-port>}

for f in "$D"/*.c; do
  [ -f "$f" ] || continue
  grep -q "asoc-618-compat.h" "$f" || \
    sed -i '0,/^#include/s//#include "asoc-618-compat.h"\n#include/' "$f"
  sed -i '
    s/struct snd_soc_codec_driver/struct snd_soc_component_driver/g
    s/struct snd_soc_platform_driver/struct snd_soc_component_driver/g
    s/snd_soc_codec_get_drvdata/snd_soc_component_get_drvdata/g
    s/snd_soc_kcontrol_codec/snd_soc_kcontrol_component/g
    s/snd_soc_dapm_to_codec/snd_soc_dapm_to_component/g
    s/snd_soc_platform_get_drvdata/snd_soc_component_get_drvdata/g
    s/snd_soc_platform_set_drvdata/snd_soc_platform_set_drvdata/g
    s/snd_soc_codec_get_dapm/snd_soc_component_get_dapm/g
    s/\.component_driver = {/\/\* component_driver dibuka ke driver induk (6.18) *\//
    s/\.reg_word_size = sizeof(u16),/\/\* reg_word_size: regmap-less codec, ops read\/write langsung (6.18) *\//
    s/\.reg_cache_step = 2,/\/\* reg_cache_step: dihapus 6.18 *\//
    s/\.compr_ops\t= \&sprd_platform_compr_ops,/\.compress_ops = \&sprd_platform_compr_ops,/
    s/\.compr_ops = \&sprd_platform_compr_ops,/.compress_ops = \&sprd_platform_compr_ops,/
    s/static struct snd_compr_ops/static struct snd_compress_ops/
    s/static int  timer_init(void);/static int timer_init(void);/
    s/\.pcm_new = sprd_pcm_new,/.pcm_construct = sprd_pcm_new,/
  ' "$f"
done
echo "sed selesai: $D"
