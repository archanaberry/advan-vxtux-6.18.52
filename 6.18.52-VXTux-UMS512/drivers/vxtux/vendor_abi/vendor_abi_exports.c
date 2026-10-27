// SPDX-License-Identifier: GPL-2.0
/*
 * drivers/vxtux/vendor_abi/vendor_abi_exports.c
 *
 * Ekspor simbol 6.18.52 yang sudah ADA di vmlinux tetapi tidak diekspor.
 *
 * DASAR (bukan asumsi)
 *   donors/symreach.py mencocokkan 5.572 referensi SHN_UNDEF dari 129 blob
 *   stok terhadap Module.symvers tick13 dan `nm --defined-only vmlinux.o`.
 *   Hasil kelas `ekspor`: 23 referensi / 19 simbol GLOBAL ada di vmlinux.o
 *   tetapi tidak punya entri EXPORT_SYMBOL. Modul hanya bisa memakai simbol
 *   yang diekspor, jadi referensi blob ke simbol-simbol ini tidak akan
 *   pernah resolve walau kodenya ada.
 *
 * MENGAPA SATU FILE, BUKAN 19 SUNTINGAN
 *   `EXPORT_SYMBOL(x)` hanya membuat entri ksymtab yang menunjuk &x; simbol x
 *   tidak harus didefinisikan di file yang sama, cukup punya linkage global
 *   saat vmlinux di-link.
 *
 *   Syarat yang sudah diverifikasi: tidak satu pun dari 19 nama ini ada di
 *   Module.symvers tick13, jadi tidak ada "exported twice" saat modpost.
 *
 * PROTOTIPE
 *   Semua di bawah diambil dari pohon ini, bukan dikarang:
 *     include/linux/irqnr.h:10, include/linux/cma.h:28-55,
 *     kernel/dma/direct.h:21,40,42,
 *     block/blk.h:577,590,
 *     drivers/vxtux/sound/asoc/card/include/sprd-audio.h:77-177,
 *     drivers/vxtux/audio/mcdt/mcdt_hw.h:142-143.
 *   Versi pertama file ini memakai `extern void nama(void)` dan gagal compile
 *   ("conflicting types for 'irq_to_desc'") — prototipe asli wajib dipakai.
 *
 * BATAS JUJUR
 *   Ini menambah ekspor, BUKAN menambah kode. Kalau salah satu simbol hilang
 *   dari pohon, modpost akan gagal dan itu memang tujuannya: deklarasi mati
 *   harus terlihat.
 */

#include <linux/export.h>
#include <linux/module.h>
#include <linux/irqnr.h>
#include <linux/cma.h>
#include <linux/dma-mapping.h>
#include <linux/blkdev.h>
#include <linux/lockdep.h>
#include <linux/regmap.h>

/* block: private header block/blk.h, jadi prototipe ditulis eksplisit. */
struct request_queue *blk_alloc_queue(struct queue_limits *lim, int node_id);
struct gendisk *__alloc_disk_node(struct request_queue *q, int node_id,
				  struct lock_class_key *lkclass);

/* dma direct: kernel/dma/direct.h tidak boleh di-include dari driver. */
int dma_direct_map_sg(struct device *dev, struct scatterlist *sgl, int nents,
		      enum dma_data_direction dir, unsigned long attrs);
void dma_direct_unmap_sg(struct device *dev, struct scatterlist *sgl,
			 int nents, enum dma_data_direction dir,
			 unsigned long attrs);
void dma_direct_sync_sg_for_cpu(struct device *dev, struct scatterlist *sgl,
				int nents, enum dma_data_direction dir);

/* audio AGCP / MCDT: helper vendor, prototipe dari header vendor. */
struct regmap *arch_audio_get_agcp_ahb_gpr(void);
u32 get_agcp_ahb_set_offset(void);
u32 get_agcp_ahb_clr_offset(void);
void set_agcp_ahb_offset(u32 set_ahb_offset, u32 clr_ahb_offset);
void arch_audio_set_pmu_apb_gpr(struct regmap *gpr);
void arch_audio_set_aon_apb_gpr(struct regmap *gpr);
void arch_audio_set_anlg_phy_g(struct regmap *gpr);
void mcdt_da_fifo_clr(unsigned int chan_num);
void mcdt_ad_fifo_clr(unsigned int chan_num);

EXPORT_SYMBOL(irq_to_desc);
EXPORT_SYMBOL(blk_alloc_queue);
EXPORT_SYMBOL(__alloc_disk_node);
EXPORT_SYMBOL(dma_direct_map_sg);
EXPORT_SYMBOL(dma_direct_unmap_sg);
EXPORT_SYMBOL(dma_direct_sync_sg_for_cpu);
EXPORT_SYMBOL(cma_alloc);
EXPORT_SYMBOL(cma_release);
EXPORT_SYMBOL(cma_for_each_area);
EXPORT_SYMBOL(cma_get_name);
EXPORT_SYMBOL(arch_audio_get_agcp_ahb_gpr);
EXPORT_SYMBOL(get_agcp_ahb_set_offset);
EXPORT_SYMBOL(get_agcp_ahb_clr_offset);
EXPORT_SYMBOL(set_agcp_ahb_offset);
EXPORT_SYMBOL(arch_audio_set_pmu_apb_gpr);
EXPORT_SYMBOL(arch_audio_set_aon_apb_gpr);
EXPORT_SYMBOL(arch_audio_set_anlg_phy_g);
EXPORT_SYMBOL(mcdt_da_fifo_clr);
EXPORT_SYMBOL(mcdt_ad_fifo_clr);
