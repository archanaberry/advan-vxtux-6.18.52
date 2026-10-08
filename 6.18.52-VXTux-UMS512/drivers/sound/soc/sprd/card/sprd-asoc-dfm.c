// SPDX-License-Identifier: GPL-2.0
/*
 * sprd-asoc-dfm.c — penyimpan priv DFM (digital filter manager) ASoC Unisoc.
 *
 * RE (unisoc/docs/blob_audit_syms.csv, kolom sym_T): blob vendor
 * snd-soc-sprd-card.ko mengekspor dfm_priv_get bersama arch_audio_* dan
 * sprd_asoc_card_*. Source GPL fork menaruh implementasinya di
 *   sound/soc/sprd/dai/vbc/r1p0v3/vbc.c:144  dfm_priv_set()  (memcpy ke global)
 * dan machine driver memakainya lewat weak-alias
 *   sound/soc/sprd/vbc-rxpx-codec-sc27xx.c:33.
 *
 * Port 6.18: satu tempat saja di modul card (pengganti blob itu), dengan
 * semantik sama seperti fork (salin parameter, kembalikan pointer global),
 * supaya permukaan ekspor blob benar-benar lengkap — bukan stub kosong.
 */
#include "../asoc-618-compat.h"
#include <linux/string.h>

#include "dfm.h"

static struct sprd_dfm_priv vxtux_dfm;

/* cf. fork dai/vbc/r1p0v3/vbc.c:144 */
void dfm_priv_set(struct sprd_dfm_priv *in_dfm)
{
	if (!in_dfm)
		return;

	memcpy(&vxtux_dfm, in_dfm, sizeof(vxtux_dfm));
}

/* Penutup sym_T blob: versi get yang tidak ada di source GPL mana pun. */
struct sprd_dfm_priv *dfm_priv_get(void)
{
	return &vxtux_dfm;
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ASoC SPRD DFM private state (VXTux port)");
