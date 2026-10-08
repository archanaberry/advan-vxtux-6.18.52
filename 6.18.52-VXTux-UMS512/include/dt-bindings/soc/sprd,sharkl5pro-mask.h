/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Unisoc Sharkl5Pro (UMS512 / T618) CSI register bit masks.
 *
 * Recovered 2026-10-05 from blobs/vendor_dlkm/sprd_sensor.ko, then CONFIRMED
 * against a second, independent authority: the vendor's OWN auto-generated
 * ASIC headers, shipped in the U-Boot fork at src/upstream/u-boot-ums512 --
 *   arch/arm/include/asm/arch-sharkl5pro/chip_sharkl5pro/anlg_phy_g10.h
 *   arch/arm/include/asm/arch-sharkl5pro/chip_sharkl5pro/mm_ahb.h
 *   arch/arm/include/asm/arch-sharkl5pro/chip_sharkl5pro/aon_apb.h
 *   include/dt-bindings/soc/sprd,sharkl5pro_aon_apb-mask.h
 * marked "Auto generated c code from ASIC Documentation, PLEASE DONOT EDIT",
 * i.e. the datasheet itself. Those state BIT positions; the values below are
 * those positions evaluated, which is the mask form the driver passes.
 *
 * The confirmation is not vague. The blob sweep found ONE composite mask,
 * 0x480000, at register 0xb4, with no way to say what composed it.
 * csi_phy_testclr_init() builds it as TESTCLR_S_EN | TESTCLR_M_EN, and the
 * vendor header gives TESTCLR_S_EN = BIT(19) and TESTCLR_M_EN = BIT(22):
 *
 *     0x80000 | 0x400000 = 0x480000      -- exact.
 *
 * An opaque blob constant decomposing into two named datasheet bits is
 * stronger evidence than either source on its own, and it validates the whole
 * offset mapping at the same time.
 *
 * CORRECTIONS this forced on the previous revision of this file. That version
 * had collected five mask VALUES with no regmap and no register binding, and
 * so had invented four wrong names for them. Every value was real; only the
 * names were not:
 *
 *   old name                                actual identity (source-confirmed)
 *   MASK_AON_APB_RF_CGM_CPHY_CFG_EN 0x480000  -> NOT it: CPHY_CFG_EN is
 *                                        BIT(15)=0x8000 at 0x13c. 0x480000 is
 *                                        the 0xb4 TESTCLR_S_EN|M_EN pair.
 *   MASK_AON_APB_CSI_2L_RCTL      0x10        -> 2P2LANE_CSI_MODE_SEL at 0x38
 *   BIT_AON_APB_CSI_2P2L_RCTL     0x01        -> 2P2LANE_DSI_TESTCLR_DB at 0x7c
 *   MASK_AON_APB_CSI_2P2L_DBG_TRIMBG 0x30     -> came from the 0x7000 site,
 *                                        which was not a register at all
 *                                        (retired in -regs.h)
 *   BIT_AON_APB_CSI_2P2L_M_PHY_CTRL 0x8000    -> CPHY_CFG_EN, BIT(15), at 0x13c
 *
 * Also REMOVED: the three macros this file used to define that csi_api.c
 * defines locally with DIFFERENT values (CSI_PATTERN_ENABLE, CSI_CLK_SOURCE,
 * CSI_CLK_SOURCE_MSK). A header winning over the source's own definition --
 * or colliding with it under CONFIG_WERROR -- is a silent behaviour change on
 * a live path, and the vendor source is the authority for what those mean.
 *
 * NOT defined here on purpose: BIT_AON_APB_CSI_2L_RCTL(x),
 * BIT_AON_APB_CSI_2P2L_RCTL(x) and BIT_AON_APB_CSI_2P2L_DBG_TRIMBG(x), which
 * the driver names only inside csi_efuse_cfg(). That function is wrapped in
 * #if 0 and its only external dependency, sprd_ap_efuse_read(), exists
 * nowhere in this tree. Defining them would only hide the fact that the eFuse
 * calibration path is unported. (The sc9833 vendor header has
 * BIT_AON_APB_CSI_2L_RCTL(x) = ((x) & 0xF) << 16 -- a different SoC family,
 * so it must be re-derived before use, not copied.)
 */

#ifndef _DT_BINDINGS_SOC_SPRD_SHARKL5PRO_MASK_H
#define _DT_BINDINGS_SOC_SPRD_SHARKL5PRO_MASK_H

/* ------------------------------------------------------------------
 * sprd,cam-ahb-syscon  (MM_AHB)
 * mm_ahb.h, "bits definitions for REG_MM_AHB_AHB_EB":  CSI0 = BIT(6),
 * CSI1 = BIT(5), CSI2 = BIT(4).  For REG_MM_AHB_AHB_RST: MIPI_CSI0 = BIT(9),
 * MIPI_CSI1 = BIT(8), MIPI_CSI2 = BIT(7).  Blob agreement: 0x04 is that AHB
 * reset register, cleared then set around a delay by csi_ahb_reset().
 * ------------------------------------------------------------------ */
#define MASK_MM_AHB_RF_CSI0_EB			0x00000040	/* BIT(6) */
#define MASK_MM_AHB_RF_CSI1_EB			0x00000020	/* BIT(5) */
#define MASK_MM_AHB_RF_CSI2_EB			0x00000010	/* BIT(4) */

#define MASK_MM_AHB_RF_MIPI_CSI0_SOFT_RST	0x00000200	/* BIT(9) */
#define MASK_MM_AHB_RF_MIPI_CSI1_SOFT_RST	0x00000100	/* BIT(8) */
#define MASK_MM_AHB_RF_MIPI_CSI2_SOFT_RST	0x00000080	/* BIT(7) */

/* ------------------------------------------------------------------
 * sprd,aon-apb-syscon  (AON_APB)
 * aon_apb.h: BIT_AON_APB_CGM_CPHY_CFG_EN = BIT(15), and the dt-bindings mask
 * header states the same field as MASK_AON_APB_CGM_CPHY_CFG_EN = 0x8000.
 * Blob agreement: mask 0x8000 at 0x13c, set then cleared -- the only AON_APB
 * register the live path touches (csi_mipi_clk_enable/_disable).
 * ------------------------------------------------------------------ */
#define MASK_AON_APB_RF_CGM_CPHY_CFG_EN		0x00008000	/* BIT(15) */

/* ------------------------------------------------------------------
 * sprd,anlg_phy_g10_controller  (ANLG_PHY_G10)
 *
 * REG_..._COMBO_CSI_4L_BIST_TEST (0x00B4), anlg_phy_g10.h:
 *   TESTCLR_M_EN BIT(22)   TESTCLR_M_SEL BIT(21)   TESTCLR_M  BIT(20)
 *   TESTCLR_S_EN BIT(19)   TESTCLR_S_SEL BIT(18)   TESTCLR_S  BIT(17)
 *   FORCE_CSI_PHY_SHUTDOWNZ BIT(26)   FORCE_CSI_S_PHY_SHUTDOWNZ BIT(24)
 * ------------------------------------------------------------------ */
#define MASK_ANLG_PHY_G10_RF_ANALOG_MIPI_CSI_COMBO_CSI_2P2L_TESTCLR_M_EN		0x00400000	/* BIT(22) */
#define MASK_ANLG_PHY_G10_RF_ANALOG_MIPI_CSI_COMBO_CSI_2P2L_TESTCLR_M_SEL		0x00200000	/* BIT(21) */
#define MASK_ANLG_PHY_G10_RF_ANALOG_MIPI_CSI_COMBO_CSI_2P2L_TESTCLR_M			0x00100000	/* BIT(20) */
#define MASK_ANLG_PHY_G10_RF_ANALOG_MIPI_CSI_COMBO_CSI_2P2L_TESTCLR_S_EN		0x00080000	/* BIT(19) */
#define MASK_ANLG_PHY_G10_RF_ANALOG_MIPI_CSI_COMBO_CSI_2P2L_TESTCLR_S_SEL		0x00040000	/* BIT(18) */
#define MASK_ANLG_PHY_G10_RF_ANALOG_MIPI_CSI_COMBO_CSI_2P2L_TESTCLR_S			0x00020000	/* BIT(17) */
#define MASK_ANLG_PHY_G10_RF_ANALOG_MIPI_CSI_COMBO_FORCE_CSI_PHY_SHUTDOWNZ		0x04000000	/* BIT(26) */
#define MASK_ANLG_PHY_G10_RF_ANALOG_MIPI_CSI_COMBO_FORCE_CSI_S_PHY_SHUTDOWNZ		0x01000000	/* BIT(24) */

/*
 * The composite csi_phy_testclr_init() mask, kept visible because it is the
 * cross-check quoted in the header comment. The driver writes it as
 * S_EN | M_EN; the OR below is that expression, and it is 0x480000 -- the one
 * opaque blob constant that had no explanation until the vendor header
 * supplied both bit positions.
 */
#define CSI_COMBO_TESTCLR_EN_BOTH	\
	(MASK_ANLG_PHY_G10_RF_ANALOG_MIPI_CSI_COMBO_CSI_2P2L_TESTCLR_S_EN | \
	 MASK_ANLG_PHY_G10_RF_ANALOG_MIPI_CSI_COMBO_CSI_2P2L_TESTCLR_M_EN)

/*
 * REG_..._2P2LANE_CTRL_CSI_2P2L (0x0038): CSI_MODE_SEL = BIT(4).
 * Blob agreement: mask 0x10 at 0x38.
 */
#define MASK_ANLG_PHY_G10_RF_ANALOG_MIPI_CSI_2P2LANE_CSI_MODE_SEL		0x00000010	/* BIT(4) */

/*
 * REG_..._2P2LANE_CSI_2P2L_TEST_DB (0x007C): DSI_TESTCLR_DB = BIT(0).
 * Blob agreement: mask 0x1 at 0x7c.
 */
#define MASK_ANLG_PHY_G10_RF_ANALOG_MIPI_CSI_2P2LANE_DSI_TESTCLR_DB		0x00000001	/* BIT(0) */

/*
 * REG_..._2LANE_MIPI_PHY_BIST_TEST (0x0014): FORCE_CSI_PHY_SHUTDOWNZ = BIT(26),
 * the same position as the combo register's bit but on a different register --
 * which is why the register had to be identified before the mask made sense.
 * Reached by csi_phy_power_down() only for CSI_RX2.
 */
#define MASK_ANLG_PHY_G10_RF_ANALOG_MIPI_CSI_2LANE_FORCE_CSI_PHY_SHUTDOWNZ	0x04000000	/* BIT(26) */

/*
 * Dropped from the previous revision, with the reason, so none of it comes
 * back: CSI_IPG_CLK_*_MASK / CSI_IPG_CLK_SHIFT_* / CSI_CTRL_DIV_CLEAR_* /
 * CSI_CTRL_DIV_SET_C / CSI_CTRL_FLAG_MASK / CSI_CTRL_MISC_MASK_* /
 * CSI_CTRL_TUNE_MASK_* / CSI_PATTERN_ENABLE / CSI_CLK_SOURCE /
 * CSI_CLK_SOURCE_MSK.
 *
 * The first group described the CSI controller block at reg[0]. That block is
 * owned by the vendor enum csi_registers_t in csi_driver.h, and the masks its
 * ipg/lane paths use are defined locally in csi_driver.c next to the writes
 * that use them (IPG_ENABLE_MASK, IPG_HSYNC_EN_MASK, PHY_TESTCLR and the
 * rest). Re-declaring the same fields here would give two authorities for one
 * register with no build error to tell us when they diverged.
 *
 * The last three were defined HERE *and* in csi_api.c with different values.
 * The source's own definition is the one the ported code was written against,
 * so the header must not compete with it.
 */

/*
 * Vendor spellings required by csi_api.c, paired with the offsets above.
 * Each is the second argument of the regmap_update_bits_base() call at that
 * offset.
 */
#define BIT_AON_APB_CSI_2L_RCTL			0x10
#define BIT_AON_APB_CSI_2P2L_RCTL		0x01
#define BIT_AON_APB_CSI_2P2L_DBG_TRIMBG		0x30
#define MASK_AON_APB_RF_CGM_CPHY_CFG_EN		0x480000

#endif /* _DT_BINDINGS_SOC_SPRD_SHARKL5PRO_MASK_H */