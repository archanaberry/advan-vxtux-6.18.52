// SPDX-License-Identifier: GPL-2.0
/*
 * sc27xx_typec.c — USB Type-C / DRP controller di PMIC SC27xx (SC2730),
 * port ke mainline 6.18 dari bedah RE blob vendor.
 *
 * STATUS SUMBER: TIDAK ADA sumber GPL modul ini di fork T618, mirror
 * UMS512, atau pohon mana pun yang tersedia (sweep B34: kelas re-only,
 * dumped/re/RE_INDEX.tsv). SEMUA isi berkas diturunkan dari bukti
 * biner + FDT stok (bukan karangan, tiap angka ada di file:offset):
 *   - dumped/re/sc27xx_typec.ko.*.dis.txt (probe/irq/disconnect/show)
 *   - .rodata modul (tabel di bawah), FDT stok typec@380.
 *
 * === PETA FIELD DRIVER-DATA (tabel .rodata @0x5a8/0x5d0/0x5f8) ===
 * 10 x u32 (0x28 B per entri — cocok jarak antar tabel):
 *   [0] type          [1] cc1_shift      [2] cc2_shift
 *   [3] state_en_off  [4] irq_clr_off    [5] mode_ctrl_off
 *   [6] state_en_bit0 [7] state_en_bit1  [8] state_mask
 *   [9] irq_pending_mask
 *   sc2730 : 2  5  0  0x0c 0x10 0x14 1    2    0x1f 0x3ff
 *   sc2721 : 1  11 6  0x00 0x04 0x08 0x20 0x40 0x08 0x7f
 *   ump9620: 3  1  12 0x0c 0x10 0x14 1    2    0x1f 0x3ff
 * Pemakaian (offset pemanggilan disasm):
 *   [0] probe mode-bit & rtrim kondisi & jalur polaritas IRQ;
 *   [1]/[2] shift kalibrasi CC1/CC2 dari efuse (probe 0x204/0x2b0);
 *   [3] reg enable state (probe 0x4c4); [4] reg clear irq (irq 0x9dc);
 *   [5] reg kontrol mode (probe 0x334/0x3f8); [8] mask state (irq
 *   0x7ec); [9] mask pending (irq 0x7c4).
 *
 * === URUTAN PROBE (disasm sc27xx_typec_probe) ===
 *   match data → extcon alloc/register → platform_get_irq →
 *   dev_get_regmap(dev->parent) → DT "reg" (base PMIC typec 0x380) →
 *   DT "sprd,mode" (0/2 ok; 1 atau >2 → only_usb20 + mode=1) →
 *   cap tertanam di struct (+0x48): type=mode, data=2(DRD),
 *   revision=0x0120, prefer_role=-1 → typec_register_port →
 *   sysfs_create_groups PADA PLATFORM DEVICE (probe 0x184-0x188) →
 *   nvmem "typec_cc1_cal"/"typec_cc2_cal" (get/read/put + kfree) →
 *   regmap_write(base+0x3c, cc1|(cc2<<5)) → devm_request_threaded_irq
 *   (handler=NULL, thread_fn, IRQF_ONESHOT|IRQF_TRIGGER_FALLING —
 *   disasm w4=0x2000|0x2; shared TIDAK ada) → read-modify-write
 *   reg base+[5]: val=(val&~3)|(type==0?1:(type==2?2:0)) →
 *   bila only_usb20 && cap.data==1: regmap_update_bits base,0x10,0x10 →
 *   bila (type&~1)==2: regmap_write(base+0x20, 0xc7f) →
 *   bila type==3: reg 0x234c |= 0x10 (UMP9620) → read-modify-write
 *   reg base+[3] |= [6]|[7] → platform_set_drvdata.
 *   (Quirk blob yang DIPORT PERSIS: kegagalan sysfs_create_groups
 *   hanya dicetak lalu probe LANJUT ke nvmem — 0x188→0x588→0x59c→0x18c.)
 *
 * === MESIN NEGARA IRQ (disasm sc27xx_typec_interrupt) ===
 *   raw = regmap_read(base+0x18); pending = raw & data[9].
 *   bit1 pending → sc27xx_typec_disconnect(); bila (type&~1)==2:
 *   tdrp = (get_random_bytes % 1600) + 1599 ms; regmap_write
 *   (base+0x48, tdrp) (syarat DRP 5.7.9); dev_info SC27XX_MIN_TERR_CNT.
 *   bit0 pending → attach path: bila sudah ada partner → skip;
 *   idx = state-2 harus ≤6 dan bit idx dari 0x69 ter-set (state
 *   2,5,7,8): state==7 → extcon_set_state_sync(edev, 0x15, true)
 *   (lihat DEV-1); selain itu: mode=tbl_mode[idx] → typec_set_pwr_
 *   opmode(port, 0), pwr/data/vconn_role(mode), lalu bila state lagi-
 *   lain memenuhi mask → extcon_set_state_sync(edev, tbl_cable[idx],
 *   tbl_bool[idx]).
 *   Kemudian polaritas CC (jalur type==1 beda register dari jalur
 *   lain): nilai polaritas = ((raw & 8) ? bit7 : bit0) ^ 1 (type==1)
 *   atau ((raw & 0x80) ? bit7 : bit0) ^ 1 (reg base+0x60, type!=1);
 *   bila berubah → sysfs_notify("typec_cc_polarity_role") +
 *   kobject_uevent(KOBJ_CHANGE), simpan ke cc_polarity_role.
 *   Penutup: regmap_write(base+[4], pending); dev_info state/event;
 *   return IRQ_HANDLED.
 *
 * === TABEL .rodata (verbatim, makna sebagian tak-transparan —
 *     diport PERSIS supaya perilaku mengikuti silikon) ===
 *   tbl_mode  @0x684: {0,1,2,1,0,0}          (idx = state-2)
 *   tbl_cable @0x668: {0,1,0,1,1,2,1,0x15}   (id kabel extcon)
 *   tbl_bool  @0x6a0: {2,2,2,5,2,7,2}        (nilai bool non-nol)
 *   mask attach 0x69 (bit idx state-2), mask kabel SC27xx 0x15/0x69.
 *   cable table extcon @0x658: {1, 2} = {EXTCON_USB, EXTCON_USB_HOST}.
 *
 * === CATATAN API 6.18 (B43) ===
 *   API penyedia extcon di 6.18 TIDAK ada di <linux/extcon.h> — ia pindah
 *   ke <linux/extcon-provider.h> (extcon.h tinggal API konsumen). B40
 *   hanya menyertakan extcon.h, sehingga devm_extcon_dev_allocate/register
 *   dan extcon_set_state_sync jadi implicit declaration: 5 error nyata
 *   (smoke clang 262/293/310/464/470; baris 639 "expected ';'" adalah
 *   lanjutan pemulihan clang dari error yang sama, bukan cacat terpisah).
 *   Include ditambahkan — tidak ada perubahan perilaku atau angka RE.
 *   Blob 5.4 memakai extcon.h 5.4 yang saat itu memang memuat keduanya.
 *
 * === DEVIASI ===
 *   DEV-1: state==7 menulis id kabel mentah 0x15 (21). Header extcon
 *     fork tidak ada di arsip sehingga makna 0x15 di pohon vendor tak
 *     terverifikasi; tabel kabel yang didaftarkan modul hanya {1,2},
 *     jadi di mainline id 21 di luar rentang dan panggilan itu
 *     kembali -EINVAL (efek: tidak ada state yang diset). Port
 *     mempertahankan panggilan verbatim (perilaku = silikon), plus
 *     catatan ini.
 *   DEV-2: devm_kmalloc blob → devm_kzalloc (semua field yang dibaca
 *     blob memang ditulis sebelum dipakai; kzalloc hanya menghapus
 *     keberuntungan). sysfs attr = platform device (bukan typec port
 *     device — struct typec_port opaque di 6.18).
 *   DEV-3: typec_cap.fwnode diisi dari anak "connector" bila ada
 *     (6.18 menuntut fwnode konsisten bila dipakai); FDT stok tidak
 *     punya anak itu → NULL, sama seperti blob (kzalloc).
 */
#include <linux/device.h>
#include <linux/err.h>
#include <linux/extcon-provider.h>
#include <linux/extcon.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/nvmem-consumer.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/random.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/usb/typec.h>

/* state bits vendor (SC27xx, dari mask & tabel — bukan enum spec) */
#define SC27XX_TYPEC_INT_DRP_TOGGLED		BIT(0)
#define SC27XX_TYPEC_INT_DISCONNECT		BIT(1)

#define SC27XX_MIN_TERR_CNT_MIN_MS	1599
#define SC27XX_MIN_TERR_CNT_RANGE_MS	1600

/* 0x69 = bitmask idx (state-2) yang boleh attach/extcon */
#define SC27XX_ATTACH_STATE_MASK	0x69

static const unsigned int sc27xx_tbl_mode[] = { 0, 1, 2, 1, 0, 0 };
static const unsigned int sc27xx_tbl_cable[] = { 0, 1, 0, 1, 1, 2, 1, 0x15 };
static const unsigned int sc27xx_tbl_bool[] = { 2, 2, 2, 5, 2, 7, 2 };

static const unsigned int sc27xx_typec_extcon_cable[] = {
	EXTCON_USB, EXTCON_USB_HOST,
};

struct sc27xx_typec_data {
	u32	type;
	u32	cc1_shift;
	u32	cc2_shift;
	u32	state_en_off;
	u32	irq_clr_off;
	u32	mode_ctrl_off;
	u32	state_en_bit0;
	u32	state_en_bit1;
	u32	state_mask;
	u32	irq_pending_mask;
};

static const struct sc27xx_typec_data sc2730_data = {
	.type		= 0x2,
	.cc1_shift	= 5,
	.cc2_shift	= 0,
	.state_en_off	= 0x0c,
	.irq_clr_off	= 0x10,
	.mode_ctrl_off	= 0x14,
	.state_en_bit0	= 1,
	.state_en_bit1	= 2,
	.state_mask	= 0x1f,
	.irq_pending_mask = 0x3ff,
};

static const struct sc27xx_typec_data sc2721_data = {
	.type		= 0x1,
	.cc1_shift	= 11,
	.cc2_shift	= 6,
	.state_en_off	= 0x00,
	.irq_clr_off	= 0x04,
	.mode_ctrl_off	= 0x08,
	.state_en_bit0	= 0x20,
	.state_en_bit1	= 0x40,
	.state_mask	= 0x08,
	.irq_pending_mask = 0x7f,
};

static const struct sc27xx_typec_data ump9620_data = {
	.type		= 0x3,
	.cc1_shift	= 1,
	.cc2_shift	= 12,
	.state_en_off	= 0x0c,
	.irq_clr_off	= 0x10,
	.mode_ctrl_off	= 0x14,
	.state_en_bit0	= 1,
	.state_en_bit1	= 2,
	.state_mask	= 0x1f,
	.irq_pending_mask = 0x3ff,
};

static const struct of_device_id typec_sprd_match[] = {
	{ .compatible = "sprd,sc2730-typec", .data = &sc2730_data },
	{ .compatible = "sprd,sc2721-typec", .data = &sc2721_data },
	{ .compatible = "sprd,ump96xx-typec", .data = &ump9620_data },
	{ .compatible = "sprd,ump9620-typec", .data = &ump9620_data },
	{ }
};
MODULE_DEVICE_TABLE(of, typec_sprd_match);

/*
 * Cap typec tertanam di dalam struct alokasi yang sama dengan blob
 * (probe 0x13c-0x168: cap = &typec->cap_field @+0x48).
 */
struct sc27xx_typec {
	struct device		*dev;		/* +0x00 */
	struct regmap		*regmap;	/* +0x08 */
	u32			reg;		/* +0x10 base PMIC dari DT */
	int			irq;		/* +0x14 */
	struct extcon_dev	*edev;		/* +0x18 */
	bool			only_usb20;	/* +0x28 */
	u32			state;		/* +0x2c (reg 0x1c ter-mask) */
	u32			last_state;	/* +0x30 */
	int			cc_polarity_role; /* +0x34 */
	struct typec_port	*port;		/* +0x38 */
	struct typec_partner	*partner;	/* +0x40 */
	struct typec_capability	cap;		/* +0x48 */
	const struct sc27xx_typec_data *data;	/* +0x98 */
};

static const char * const typec_cc_polarity_roles[] = {
	"invalid role", "cc_1", "cc_2",
};

static ssize_t typec_cc_polarity_role_show(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct sc27xx_typec *typec = dev_get_drvdata(dev);

	if (typec->cc_polarity_role < 0 ||
	    typec->cc_polarity_role > 2)
		return sprintf(buf, "%s\n", typec_cc_polarity_roles[0]);

	return sprintf(buf, "%s\n",
		       typec_cc_polarity_roles[typec->cc_polarity_role]);
}
static DEVICE_ATTR(typec_cc_polarity_role, 0444,
		   typec_cc_polarity_role_show, NULL);

static struct attribute *sc27xx_typec_attrs[] = {
	&dev_attr_typec_cc_polarity_role.attr,
	NULL,
};

static const struct attribute_group sc27xx_typec_group = {
	.attrs = sc27xx_typec_attrs,
};

static const struct attribute_group *sc27xx_typec_groups[] = {
	&sc27xx_typec_group,
	NULL,
};

static void sc27xx_typec_disconnect(struct sc27xx_typec *typec)
{
	unsigned int idx, cable;

	typec_unregister_partner(typec->partner);
	typec->partner = NULL;
	typec_set_pwr_opmode(typec->port, TYPEC_PWR_MODE_USB);
	typec_set_pwr_role(typec->port, TYPEC_SINK);
	typec_set_data_role(typec->port, TYPEC_DEVICE);
	typec_set_vconn_role(typec->port, TYPEC_SINK);

	/* disasm disconnect 0x6ac-0x6dc: idx = last_state - 2, mask
	 * 0x69, id kabel dari tbl_cable, state = false. */
	if (typec->last_state < 2)
		return;
	idx = typec->last_state - 2;
	if (idx > 6)
		return;
	if (!((SC27XX_ATTACH_STATE_MASK >> idx) & 1))
		return;
	cable = sc27xx_tbl_cable[idx];
	extcon_set_state_sync(typec->edev, cable, false);
}

static int sc27xx_typec_attach(struct sc27xx_typec *typec, u32 state)
{
	struct typec_partner_desc desc;
	unsigned int idx, mode, cable, val;
	int ret;

	memset(&desc, 0, sizeof(desc));
	desc.identity = NULL;
	typec->partner = typec_register_partner(typec->port, &desc);
	if (!typec->partner) {
		dev_err(typec->dev, "failed to register partner\n");
		return -ENODEV;
	}

	/* disasm irq 0x8b0-0x8e4: idx = state-2, wajib ≤ 6 dan lolos
	 * mask 0x69; selain itu lanjut ke polaritas tanpa attach. */
	if (state < 2)
		return 0;
	idx = state - 2;
	if (idx > 6)
		return 0;
	if (!((SC27XX_ATTACH_STATE_MASK >> idx) & 1))
		return 0;

	if (state == 0x7) {
		/* disasm 0x8e8-0x908: state ATTACHED_SRC → extcon id
		 * 0x15 (DEV-1), tanpa typec_set_* role. */
		typec->last_state = state;
		extcon_set_state_sync(typec->edev, 0x15, true);
		return 0;
	}

	mode = sc27xx_tbl_mode[idx];
	/* disasm 0x920: pwr_opmode SELALU 0 (TYPEC_PWR_MODE_USB). */
	typec_set_pwr_opmode(typec->port, TYPEC_PWR_MODE_USB);
	typec_set_pwr_role(typec->port, mode);
	typec_set_data_role(typec->port, mode);
	typec_set_vconn_role(typec->port, mode);

	/* disasm 0x948-0x960 + 0xa7c-0xa98: extcon dari dua tabel. */
	if (!((SC27XX_ATTACH_STATE_MASK >> idx) & 1))
		return 0;
	val = sc27xx_tbl_bool[idx];
	cable = sc27xx_tbl_cable[idx];
	typec->last_state = val;
	ret = extcon_set_state_sync(typec->edev, cable, true);
	if (ret && ret != -EINVAL)
		dev_warn(typec->dev, "failed to set cable state %d\n", ret);
	return 0;
}

static irqreturn_t sc27xx_typec_interrupt(int irq, void *dev_id)
{
	struct sc27xx_typec *typec = dev_id;
	const struct sc27xx_typec_data *data = typec->data;
	u32 event = 0, state, tdrp, val, polarity;
	int ret;

	/* disasm 0x774-0x7cc: raw pending = reg(base+0x18) & data[9]. */
	ret = regmap_read(typec->regmap, typec->reg + 0x18, &event);
	if (ret)
		return IRQ_NONE;
	event &= data->irq_pending_mask;

	/* disasm 0x7d0-0x7f4: state = reg(base+0x1c); simpan versi
	 * ter-mask data[8] ke typec->state (persis blob). */
	ret = regmap_read(typec->regmap, typec->reg + 0x1c, &state);
	if (ret)
		goto out_clear;
	typec->state = typec->state & data->state_mask;

	/* bit1 = disconnect (disasm 0x7f8-0x808). */
	if (event & SC27XX_TYPEC_INT_DISCONNECT) {
		sc27xx_typec_disconnect(typec);

		/* disasm 0x80c-0x88c: tdrp acak 1599..3198 ms, ditulis
		 * ke base+0x48 bila (type&~1)==2; log MIN_TERR_CNT. */
		/* disasm 0x80c-0x88c: tdrp acak 1599..3198 ms, ditulis
		 * ke base+0x48 bila (type&~1)==2; log MIN_TERR_CNT.
		 * (blob memakai get_random_bytes(4) & 0xfff; 6.18: bentuk
		 * get_random_u32 dipertahankan agar sisa modulo sama.) */
		val = get_random_u32();
		tdrp = (val % SC27XX_MIN_TERR_CNT_RANGE_MS) +
		       SC27XX_MIN_TERR_CNT_MIN_MS;
		if ((data->type & ~0x1) == 0x2)
			regmap_write(typec->regmap, typec->reg + 0x48, tdrp);
		dev_info(typec->dev, "SC27XX_MIN_TERR_CNT = %d: %d ms\n",
			 tdrp, (tdrp - SC27XX_MIN_TERR_CNT_MIN_MS) >> 5);
		goto out_clear;
	}

	/* bit0 = attach/state change (disasm 0x7f8-0x8a0). */
	if (event & SC27XX_TYPEC_INT_DRP_TOGGLED) {
		if (typec->partner)
			goto out_polarity;
		sc27xx_typec_attach(typec, state);
	}

out_polarity:
	/* Polaritas CC (disasm 0x964-0xa78). type==1 membaca ulang reg
	 * state (base+0x1c); lainnya reg base+0x60. */
	if (data->type == 0x1) {
		ret = regmap_read(typec->regmap, typec->reg + 0x1c, &val);
		if (ret) {
			dev_err(typec->dev, "failed to read STATUS register.\n");
			goto out_clear;
		}
		/* disasm 0xa08-0xa48: bit3 → indeks 0; selain itu ambil
		 * bit7 (raw) sebagai kandidat, dibandingkan ^ 1. */
		polarity = (val & 0x8) ? 0 : ((val & 0x80) >> 7) ^ 1;
		polarity ^= 1;
		polarity &= 0x1;
	} else {
		ret = regmap_read(typec->regmap, typec->reg + 0x60, &val);
		if (ret) {
			dev_err(typec->dev, "failed to read DBG1 register.\n");
			goto out_clear;
		}
		polarity = (val & 0x80) ? 1 : (val & 0x1);
		polarity ^= 1;
	}
	if (typec->cc_polarity_role != polarity) {
		sysfs_notify(&typec->dev->kobj, NULL,
			     "typec_cc_polarity_role");
		kobject_uevent(&typec->dev->kobj, KOBJ_CHANGE);
		typec->cc_polarity_role = polarity;
	}

out_clear:
	/* disasm 0x9cc-0xa04: clear pending via reg base+[4], log
	 * state/event, IRQ_HANDLED. */
	regmap_write(typec->regmap, typec->reg + data->irq_clr_off, event);
	dev_info(typec->dev,
		 "now works as DRP and is in %d state, event %d\n",
		 typec->state, event);
	return IRQ_HANDLED;
}

/* Kalibrasi CC — bit-math disasm dipertahankan verbatim:
 *   cc1 (probe 0x200-0x230): shift = data[1];
 *     res = (val & (0xffffffff >> (54 - shift)) &
 *            (0xffffffff << (shift + 5))) >> shift
 *   cc2 (probe 0x2a0-0x2d0): shift = data[2];
 *     res = (val & (0xffffffff << shift) &
 *            (0xffffffff >> (59 - shift))) >> shift
 *   tulis reg(base+0x3c) = cc1_res | (cc2_res << 5) (0x2d8-0x2ec). */
static int sc27xx_typec_trim(struct sc27xx_typec *typec,
			     const char *cell_name, u32 shift,
			     bool is_cc2, u32 *out)
{
	struct nvmem_cell *cell;
	size_t len = 0;
	void *buf;
	u32 val = 0, res, mask_a, mask_b;

	cell = nvmem_cell_get(typec->dev, cell_name);
	if (IS_ERR(cell))
		return PTR_ERR(cell);

	buf = nvmem_cell_read(cell, &len);
	nvmem_cell_put(cell);
	if (IS_ERR(buf))
		return PTR_ERR(buf);
	memcpy(&val, buf, min(len, sizeof(val)));
	kfree(buf);

	if (is_cc2) {
		mask_a = 0xffffffffU << shift;
		mask_b = 0xffffffffU >> (59 - shift);
	} else {
		mask_a = 0xffffffffU << (shift + 5);
		mask_b = 0xffffffffU >> (54 - shift);
	}
	res = (val & mask_a & mask_b) >> shift;
	*out = is_cc2 ? res << 5 : res;
	return 0;
}

static int sc27xx_typec_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	const struct sc27xx_typec_data *data;
	struct sc27xx_typec *typec;
	u32 mode = 0, cc1 = 0, cc2 = 0, val;
	int ret;

	data = of_device_get_match_data(dev);
	if (!data) {
		dev_err(dev, "No matching driver data found\n");
		return -EINVAL;
	}

	typec = devm_kzalloc(dev, sizeof(*typec), GFP_KERNEL);
	if (!typec)
		return -ENOMEM;
	typec->dev = dev;
	typec->data = data;

	/* disasm 0x8c-0xb8: extcon pertama, sebelum irq/regmap. */
	typec->edev = devm_extcon_dev_allocate(dev,
					       sc27xx_typec_extcon_cable);
	if (IS_ERR(typec->edev)) {
		dev_err(dev, "failed to allocate extcon device\n");
		return PTR_ERR(typec->edev);
	}
	ret = devm_extcon_dev_register(dev, typec->edev);
	if (ret) {
		dev_err(dev, "can't register extcon device: %d\n", ret);
		return ret;
	}

	typec->irq = platform_get_irq(pdev, 0);
	if (typec->irq < 0) {
		dev_err(dev, "failed to get typec interrupt.\n");
		return typec->irq;
	}

	typec->regmap = dev_get_regmap(dev->parent, NULL);
	if (!typec->regmap) {
		dev_err(dev, "failed to get regmap.\n");
		return -ENODEV;
	}

	/* DT "reg" = base register typec di PMIC (0x380 pada FDT stok;
	 * anak-PMIC memakai reg = offset, bukan alamat memori). */
	ret = of_property_read_variable_u32_array(dev->of_node, "reg",
						  &typec->reg, 1, 1);
	if (ret < 0) {
		dev_err(dev, "failed to get reg offset!\n");
		return ret;
	}

	ret = of_property_read_variable_u32_array(dev->of_node, "sprd,mode",
						  &mode, 1, 1);
	if (ret < 0) {
		dev_err(dev, "failed to get typec port mode type\n");
		return ret;
	}
	/* disasm 0x128-0x56c: mode 0/2 sah; mode 1 atau >2 → hanya
	 * USB 2.0 (only_usb20) dan mode dipaksa 1. */
	if (mode > 2 || mode == 1) {
		dev_info(dev, "usb 2.0 only is enabled\n");
		mode = 1;
		typec->only_usb20 = true;
	}

	/* cap tertanam (disasm 0x13c-0x164): type=mode, data=2 (DRD),
	 * revision=0x0120 ("1.2"), prefer_role=-1. */
	typec->cap.type = mode;
	typec->cap.data = TYPEC_PORT_DRD;
	typec->cap.revision = USB_TYPEC_REV_1_2;
	typec->cap.prefer_role = TYPEC_NO_PREFERRED_ROLE;

	typec->port = typec_register_port(dev, &typec->cap);
	if (IS_ERR(typec->port)) {
		dev_err(dev, "failed to register port!\n");
		return -ENODEV;
	}

	/* Quirk blob dipertahankan: kegagalan groups hanya dicetak dan
	 * probe LANJUT (disasm 0x188 → 0x588 → 0x59c → 0x18c). */
	ret = sysfs_create_groups(&pdev->dev.kobj, sc27xx_typec_groups);
	if (ret)
		dev_err(dev, "failed to create cc_polarity %d\n", ret);

	ret = sc27xx_typec_trim(typec, "typec_cc1_cal", data->cc1_shift,
				false, &cc1);
	if (ret)
		goto err_unreg_port;
	ret = sc27xx_typec_trim(typec, "typec_cc2_cal", data->cc2_shift,
				true, &cc2);
	if (ret)
		goto err_unreg_port;

	ret = regmap_write(typec->regmap, typec->reg + 0x3c, cc1 | cc2);
	if (ret) {
		dev_err(dev, "failed to set typec rtrim %d\n", ret);
		goto err_unreg_port;
	}

	/* disasm 0x2f0-0x320: handler=NULL + thread_fn, flags
	 * 0x2000|0x2 = IRQF_ONESHOT | IRQF_TRIGGER_FALLING. */
	ret = devm_request_threaded_irq(dev, typec->irq, NULL,
					sc27xx_typec_interrupt,
					IRQF_ONESHOT | IRQF_TRIGGER_FALLING,
					"sc27xx-typec", typec);
	if (ret) {
		dev_err(dev, "failed to request irq %d\n", ret);
		goto err_unreg_port;
	}

	/* mode ctrl: read-modify-write base+[5], val&~3 | mode-bit
	 * (disasm 0x324-0x408): type 0 → bit0, type 2 → bit1, type 1 →
	 * tanpa bit. */
	ret = regmap_read(typec->regmap, typec->reg + data->mode_ctrl_off,
			  &val);
	if (ret)
		goto err_unreg_port;
	val &= ~0x3;
	if (data->type == 0x2)
		val |= 0x2;
	else if (data->type == 0x0)
		val |= 0x1;
	ret = regmap_write(typec->regmap, typec->reg + data->mode_ctrl_off,
			   val);
	if (ret)
		goto err_unreg_port;

	/* disasm 0x41c-0x444: only_usb20 && cap.data==1 → set bit 0x10
	 * pada reg base (update_bits, lazy disabled). */
	if (typec->only_usb20 && typec->cap.data == 0x1) {
		ret = regmap_update_bits_base(typec->regmap, typec->reg,
					      0x10, 0x10, NULL, false, false);
		if (ret)
			goto err_unreg_port;
	}

	/* rtrim (disasm 0x44c-0x478): (type&~1)==2 → base+0x20 = 0xc7f.
	 * sc2730 (type 2) dan ump9620 (type 3) masuk cabang ini. */
	if ((data->type & ~0x1) == 0x2) {
		ret = regmap_write(typec->regmap, typec->reg + 0x20, 0xc7f);
		if (ret)
			goto err_unreg_port;
	}

	/* UMP9620 (type 3): reg absolut 0x234c |= 0x10 (0x484-0x4b8). */
	if (data->type == 0x3) {
		ret = regmap_read(typec->regmap, 0x234c, &val);
		if (ret)
			goto err_unreg_port;
		ret = regmap_write(typec->regmap, 0x234c, val | 0x10);
	}

	/* state enable: read-modify-write base+[3] |= [6]|[7]
	 * (disasm 0x4c0-0x508). */
	ret = regmap_read(typec->regmap, typec->reg + data->state_en_off,
			  &val);
	if (ret)
		goto err_unreg_port;
	val |= data->state_en_bit0 | data->state_en_bit1;
	ret = regmap_write(typec->regmap, typec->reg + data->state_en_off,
			   val);
	if (ret)
		goto err_unreg_port;

	platform_set_drvdata(pdev, typec);
	return 0;

err_unreg_port:
	sysfs_remove_groups(&pdev->dev.kobj, sc27xx_typec_groups);
	typec_unregister_port(typec->port);
	return ret;
}

static void sc27xx_typec_remove(struct platform_device *pdev)
{
	struct sc27xx_typec *typec = platform_get_drvdata(pdev);

	sysfs_remove_groups(&pdev->dev.kobj, sc27xx_typec_groups);
	typec_unregister_port(typec->port);
}

static struct platform_driver sc27xx_typec_driver = {
	.driver = {
		.name		= "sc27xx-typec",
		.of_match_table	= typec_sprd_match,
	},
	.probe	= sc27xx_typec_probe,
	.remove	= sc27xx_typec_remove,
};
module_platform_driver(sc27xx_typec_driver);

MODULE_DESCRIPTION("Unisoc SC27xx Type-C port controller (RE port)");
MODULE_AUTHOR("LinDroid/VXTux v1.0");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sc27xx-typec");
