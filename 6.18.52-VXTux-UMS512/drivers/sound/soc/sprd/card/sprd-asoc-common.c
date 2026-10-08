/*
 * sound/soc/sprd/sprd-asoc-common.c
 *
 * SPRD ASoC Common implement -- SpreadTrum ASOC Common.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include "../asoc-618-compat.h"
#include "sprd-asoc-debug.h"
#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) pr_sprd_fmt(" COM ")""fmt

#include <linux/atomic.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <sound/soc.h>
#include <sound/info.h>

#include "sprd-asoc-common.h"

/* spreadtrum audio debug */
static int sp_audio_debug_flag = SP_AUDIO_DEBUG_DEFAULT;

struct regmap *aon_apb_gpr;

u32 agcp_ahb_set_offset;

u32 agcp_ahb_clr_offset;

struct regmap *g_agcp_ahb_gpr;
/*
 * pmu apb global registers operating interfaces
 */
struct regmap *pmu_apb_gpr;

/*
 * pmu completement apb global registers operating interfaces
 */
struct regmap *pmu_com_apb_gpr;

/*
 * ap apb global registers operating interfaces
 * ap_apb_gpr will be set by i2s.c in its probe func.
 */
struct regmap *g_ap_apb_gpr;

/* anlg_phy_g_controller */
struct regmap *anlg_phy_g;


/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
inline int get_sp_audio_debug_flag(void);
inline int get_sp_audio_debug_flag(void)
{
	return sp_audio_debug_flag;
}
EXPORT_SYMBOL(get_sp_audio_debug_flag);

static void snd_pcm_sprd_debug_read(struct snd_info_entry *entry,
				    struct snd_info_buffer *buffer)
{
	int *p_sp_audio_debug_flag = entry->private_data;

	snd_iprintf(buffer, "0x%08x\n", *p_sp_audio_debug_flag);
}

static void snd_pcm_sprd_debug_write(struct snd_info_entry *entry,
				     struct snd_info_buffer *buffer)
{
	int *p_sp_audio_debug_flag = entry->private_data;
	char line[64];
	unsigned long long flag;

	if (!snd_info_get_line(buffer, line, sizeof(line))) {
		if (kstrtoull(line, 16, &flag) != 0) {
			pr_err("ERR: %s kstrtoull failed!\n", __func__);
			return;
		}
		*p_sp_audio_debug_flag = (int)flag;
	}
}

/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
int sprd_audio_debug_init(struct snd_card *card);
int sprd_audio_debug_init(struct snd_card *card)
{
	struct snd_info_entry *entry;

	entry = snd_info_create_card_entry(card, "asoc-sprd-debug",
							card->proc_root);
	if (entry != NULL) {
		entry->c.text.read = snd_pcm_sprd_debug_read;
		entry->c.text.write = snd_pcm_sprd_debug_write;
		entry->mode |= 0200;
		entry->private_data = &sp_audio_debug_flag;
		if (snd_info_register(entry) < 0) {
			snd_info_free_entry(entry);
			entry = NULL;
		}
	}

	return 0;
}
EXPORT_SYMBOL(sprd_audio_debug_init);

/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
int aon_apb_gpr_null_check(void);
int aon_apb_gpr_null_check(void)
{
	if (aon_apb_gpr == NULL) {
		pr_err("ERR: %saon_apb_gpr is not initialized!\n",
			__func__);
		return -EINVAL;
	}

	return 0;
}

/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
void arch_audio_set_aon_apb_gpr(struct regmap *gpr);
void arch_audio_set_aon_apb_gpr(struct regmap *gpr)
{
	aon_apb_gpr = gpr;
}

/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
struct regmap *arch_audio_get_aon_apb_gpr(void);
struct regmap *arch_audio_get_aon_apb_gpr(void)
{
	return aon_apb_gpr;
}

/*
 * agcp ahb global registers operating interfaces
 */

/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
void set_agcp_ahb_offset(u32 set_ahb_offset, u32 clr_ahb_offset);
void set_agcp_ahb_offset(u32 set_ahb_offset, u32 clr_ahb_offset)
{
	agcp_ahb_set_offset = set_ahb_offset;
	agcp_ahb_clr_offset = clr_ahb_offset;
	pr_info("%s agcp_ahb_set_offset 0x%x, agcp_ahb_clr_offset 0x%x", __func__,
		agcp_ahb_set_offset, agcp_ahb_clr_offset);
}

/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
int agcp_ahb_gpr_null_check(void);
int agcp_ahb_gpr_null_check(void)
{
	if (g_agcp_ahb_gpr == NULL) {
		pr_err("ERR: %s g_agcp_ahb_gpr isn't initialized!\n",
			__func__);
		return -1;
	}

	return 0;
}



/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
void arch_audio_set_agcp_ahb_gpr(struct regmap *gpr);
void arch_audio_set_agcp_ahb_gpr(struct regmap *gpr)
{
	g_agcp_ahb_gpr = gpr;
}

/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
struct regmap *arch_audio_get_agcp_ahb_gpr(void);
struct regmap *arch_audio_get_agcp_ahb_gpr(void)
{
	return g_agcp_ahb_gpr;
}


/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
int pmu_apb_gpr_null_check(void);
int pmu_apb_gpr_null_check(void)
{
	if (pmu_apb_gpr == NULL) {
		pr_err("ERR: %s pmu_apb_gpr is not initialized!\n",
			__func__);
		return -1;
	}

	return 0;
}

/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
void arch_audio_set_pmu_apb_gpr(struct regmap *gpr);
void arch_audio_set_pmu_apb_gpr(struct regmap *gpr)
{
	pmu_apb_gpr = gpr;
}


/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
int pmu_com_apb_gpr_null_check(void);
int pmu_com_apb_gpr_null_check(void)
{
	if (pmu_com_apb_gpr == NULL) {
		pr_err("ERR: %s pmu_com_apb_gpr is not initialized!\n",
			__func__);
		return -1;
	}

	return 0;
}

/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
void arch_audio_set_pmu_com_apb_gpr(struct regmap *gpr);
void arch_audio_set_pmu_com_apb_gpr(struct regmap *gpr)
{
	pmu_com_apb_gpr = gpr;
}

/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
int ap_apb_gpr_null_check(void);
int ap_apb_gpr_null_check(void)
{
	if (g_ap_apb_gpr == NULL) {
		pr_err("ERR: %s g_ap_apb_gpr is not initialized!\n",
			__func__);
		return -1;
	}

	return 0;
}

/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
void arch_audio_set_ap_apb_gpr(struct regmap *gpr);
void arch_audio_set_ap_apb_gpr(struct regmap *gpr)
{
	g_ap_apb_gpr = gpr;
}

/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
struct regmap *arch_audio_get_ap_apb_gpr(void);
struct regmap *arch_audio_get_ap_apb_gpr(void)
{
	return g_ap_apb_gpr;
}
/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
int anlg_phy_g_null_check(void);
int anlg_phy_g_null_check(void)
{
	if (anlg_phy_g == NULL) {
		pr_err("ERR: %s anlg_phy_g2 is not initialized!\n",
			__func__);
		return -EFAULT;
	}

	return 0;
}

/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
void arch_audio_set_anlg_phy_g(struct regmap *gpr);
void arch_audio_set_anlg_phy_g(struct regmap *gpr)
{
	anlg_phy_g = gpr;
}

/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
struct regmap *arch_audio_get_anlg_phy_g(void);
struct regmap *arch_audio_get_anlg_phy_g(void)
{
	return anlg_phy_g;
}

/*
 * Penutup peran blob snd-soc-sprd-card.ko (RE: sym_T di
 * unisoc/docs/blob_audit_syms.csv). Blob 5.4 mengekspor 4 pengambil ini
 * berdampingan dengan penyetelnya, tetapi source GPL fork hanya memuat
 * penyetelnya — jadi di sini dilengkapi supaya port benar-benar menyediakan
 * SELURUH permukaan ekspor blob (bukti gate C5: T 51/51, bukan 45/51).
 */
/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
struct regmap *arch_audio_get_pmu_apb_gpr(void);
struct regmap *arch_audio_get_pmu_apb_gpr(void)
{
	return pmu_apb_gpr;
}

/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
struct regmap *arch_audio_get_pmu_com_apb_gpr(void);
struct regmap *arch_audio_get_pmu_com_apb_gpr(void)
{
	return pmu_com_apb_gpr;
}

/* Dipakai snd-soc-sprd-codec-sc2730.ko + snd-soc-sprd-vbc-v4.ko (kolom
 * sym_U audit) untuk membaca offset global AGCP yang dipasang kartu. */
/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
u32 get_agcp_ahb_set_offset(void);
u32 get_agcp_ahb_set_offset(void)
{
	return agcp_ahb_set_offset;
}

/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
u32 get_agcp_ahb_clr_offset(void);
u32 get_agcp_ahb_clr_offset(void)
{
	return agcp_ahb_clr_offset;
}

/*
 * Kelas "audio" — di fork di sound/audio_class.c (obj-$(CONFIG_SOUND)); di
 * 6.18 kita tidak boleh menambah berkas di sound/ (aturan port: hanya
 * drivers/vxtux), jadi kelas dibuat di sini (dipakai headset sc2730 untuk
 * sysfs link "earjack"). Built-in: tidak pernah unregister.
 */
struct class *audio_class;
EXPORT_SYMBOL_GPL(audio_class);

static int __init sprd_audio_class_init(void)
{
	audio_class = class_create("audio");
	if (IS_ERR(audio_class)) {
		pr_err("%s: register audio class failed, ret = %ld\n",
		       __func__, PTR_ERR(audio_class));
		return PTR_ERR(audio_class);
	}

	return 0;
}
subsys_initcall(sprd_audio_class_init);

