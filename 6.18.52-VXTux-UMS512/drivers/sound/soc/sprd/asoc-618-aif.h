/* SPDX-License-Identifier: GPL-2.0
 *
 * asoc-618-aif.h — pemulih nama DAPM intercon (aif_name) yang dihapus dari
 * struct snd_soc_pcm_stream di 6.18.
 *
 * Dibuat: unisoc/work/gen_aif_table.sh dari fork GPL T618
 *   dai/vbc/v4/{vbc-dai.c,sprd-fe-dai.c,vbc-codec.c}, codec/sprd/sc2730/sprd-codec.c
 * Konsumen: drivers/vxtux/sound/asoc/{dai_v4,codec_sc2730}.
 *
 * Baris aif_name di sumber fork: 110; pengecualian pola: 4.
 * Pola: BE_DAI_* -> BE_IF_*, FE_DAI_* -> FE_IF_* (sisanya di tabel).
 */
#ifndef _VXTUX_ASOC_618_AIF_H
#define _VXTUX_ASOC_618_AIF_H

#include <linux/kernel.h>
#include <linux/string.h>

struct vxtux_aif_exception {
	const char *stream;	/* playback/capture stream_name di dai_driver */
	const char *aif;		/* nama intercon versi vendor */
};

static const struct vxtux_aif_exception vxtux_aif_exceptions[] = {
	{ "BE_DAI_FM_CAP_C", "BE_IF_CAP_FM_CAP_C" }, /* vbc-dai.c */
	{ "BE_DAI_ID_NORMAL_AP01_P_HIFI", "BE_IF_NORMAL_AP01_HIFI_P" }, /* vbc-dai.c */
	{ "BE_DAI_ID_NORMAL_AP01_P_SMTPA", "BE_IF_NORMAL_AP01_SMTPA_P" }, /* vbc-dai.c */
	{ "DisplayPort MultiMedia Playback", "DP_DL" }, /* sprd-fe-dai.c */
};

/*
 * vxtux_aif_name() — turunkan nama intercon DAPM dari stream_name.
 * NULL = vendor juga tidak punya aif untuk stream itu (jalur DAPM dilewati,
 * sama seperti kondisi asli `... && aif_name`). Buffer statis bergantian:
 * hasilnya dipakai sebagai string DAPM yang harus hidup selama proses.
 */
static inline const char *vxtux_aif_name(const char *stream_name)
{
	static char buf[2][96];
	static int idx;
	int i;

	if (!stream_name || stream_name[0] == '\0')
		return NULL;

	for (i = 0; i < ARRAY_SIZE(vxtux_aif_exceptions); i++)
		if (!strcmp(stream_name, vxtux_aif_exceptions[i].stream))
			return vxtux_aif_exceptions[i].aif;

	if (!strncmp(stream_name, "BE_DAI_", 7))
		snprintf(buf[idx], sizeof(buf[0]), "BE_IF_%s", stream_name + 7);
	else if (!strncmp(stream_name, "FE_DAI_", 7))
		snprintf(buf[idx], sizeof(buf[0]), "FE_IF_%s", stream_name + 7);
	else
		return NULL;

	i = idx;
	idx ^= 1;
	return buf[i];
}

#endif /* _VXTUX_ASOC_618_AIF_H */
