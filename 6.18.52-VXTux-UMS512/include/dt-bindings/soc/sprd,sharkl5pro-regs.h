/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Unisoc Sharkl5Pro (UMS512 / T618) CSI receiver register map.
 *
 * Recovered 2026-10-05 from blobs/vendor_dlkm/sprd_sensor.ko by reverse
 * engineering, not written from documentation. Every offset below was read
 * straight out of the binary: either a struct field displacement in an
 * indexed access, or an immediate loaded as an argument to phy_write() or
 * regmap_update_bits_base(). Nothing here was inferred from surrounding
 * context, and no value was attributed to a register by guesswork.
 *
 * Sources for each group are in docs/RE-SPRD-SENSOR-CSI-PHY.md.
 *
 * Every base address comes from the device tree, not from this header.
 * csi_api_dt_node_init() does:
 *   of_address_to_resource(node, 0, &res) + devm_ioremap()   -> reg[0]
 *   syscon_regmap_lookup_by_phandle(node, "sprd,cam-ahb-syscon")
 *   syscon_regmap_lookup_by_phandle(node, "sprd,aon-apb-syscon")
 *   syscon_regmap_lookup_by_phandle(node, "sprd,anlg_phy_g10_controller")
 * so the three blocks below are relative to whatever the DT points at.
 *
 * Target confirmed against the live device tree, which declares three
 * controllers: csi00@62300000, csi01@62400000 and csi02@62500000, each
 * compatible "sprd,csi-controller" with sprd,csi-id 0, 1 and 2 and a 1 MB
 * window, plus mipi-csi-phy0/1/2 carrying sprd,phyid.
 *
 * Note that the driver consumes this header by NAME while the hardware
 * consumes it by OFFSET. Field names below are descriptive labels for the
 * measured positions; only the positions are load-bearing.
 */

#ifndef _DT_BINDINGS_SOC_SPRD_SHARKL5PRO_REGS_H
#define _DT_BINDINGS_SOC_SPRD_SHARKL5PRO_REGS_H

/* ------------------------------------------------------------------
 * CSI controller block, ioremap'd from reg[0].
 *
 * Struct field displacements observed in csi_ipg_mode_cfg @0x0800c758.
 * All multiples of 4, consistent with a struct of u32 fields.
 * ------------------------------------------------------------------ */
#define CSI_CTRL_ENABLE_0		0x08	/* |= 1                     */
#define CSI_CTRL_ENABLE_1		0x0c	/* |= 1                     */
#define CSI_CTRL_ENABLE_2		0x10	/* |= 1                     */

/* IPG clock mode: bitfields 21..29, 13..20 and 4..12 written by BFI. */
#define CSI_CTRL_IPG_CFG		0x14

/* Three consecutive divider registers, each with fields at 10..21,
 * 22..31 and 0..9. csi_ipg_mode_cfg clears and sets them in sequence. */
#define CSI_CTRL_DIV_0			0x50
#define CSI_CTRL_DIV_1			0x54
#define CSI_CTRL_DIV_2			0x58

#define CSI_CTRL_FLAG			0x5c	/* &= ~0x3                  */

#define CSI_CTRL_TUNE_0			0x6c	/* |= 0x1ffe000, |= 0x1fff  */

#define CSI_CTRL_MISC			0x90	/* &= ~0xc, &= ~0x3         */

/* ------------------------------------------------------------------
 * Analog PHY block.
 *
 * Argument literals at the phy_write() call sites in csi_phy_init
 * @0x0800df48. phy_write() is a module-local symbol, so these are direct
 * branches rather than relocations.
 * ------------------------------------------------------------------ */
#define CSI_PHY_CTRL			0x01

/*
 * Per-lane block. The stride is 0x10, observed directly:
 *   lane 0 -> 0x3d   lane 1 -> 0x4d   lane 2 -> 0x5d
 *   lane 3 -> 0x6d   lane 4 -> 0x7d
 */
#define CSI_PHY_LANE_BASE		0x3d
#define CSI_PHY_LANE_STRIDE		0x10
#define CSI_PHY_LANE(n)			(CSI_PHY_LANE_BASE + (n) * CSI_PHY_LANE_STRIDE)

/* Non-lane PHY registers written with lane-select or mask arguments. */
#define CSI_PHY_SEL_A			0x60
#define CSI_PHY_SEL_B			0x62
#define CSI_PHY_RST_CTRL		0x74
#define CSI_PHY_CLK_CTRL		0x87
#define CSI_PHY_MODE_A			0xa8
#define CSI_PHY_MODE_B			0xe8
#define CSI_PHY_GLOBAL_0		0xf1
#define CSI_PHY_GLOBAL_1		0xf4	/* appears as -0xc in the call */

#define CSI_PHY_CTRL_1			0x08

/* ------------------------------------------------------------------
 * Syscon blocks -- the three regmaps the driver looks up from DT.
 *
 * ATTRIBUTION CORRECTION. The blob sweep collected these offsets without
 * recording which regmap each site used, so all of them were filed under one
 * "AON_APB" bucket. Two independent sources now bind each offset to its block,
 * and they agree with each other and with the source:
 *
 *   a) csi_driver.c / csi_api.c name the regmap at every call site, and
 *   b) the vendor's OWN auto-generated ASIC headers, shipped in the U-Boot
 *      fork at src/upstream/u-boot-ums512 -- marked "Auto generated c code
 *      from ASIC Documentation, PLEASE DONOT EDIT", i.e. the datasheet:
 *        arch/arm/include/asm/arch-sharkl5pro/chip_sharkl5pro/mm_ahb.h
 *        arch/arm/include/asm/arch-sharkl5pro/chip_sharkl5pro/anlg_phy_g10.h
 *        arch/arm/include/asm/arch-sharkl5pro/chip_sharkl5pro/aon_apb.h
 *        include/dt-bindings/soc/sprd,sharkl5pro_aon_apb-regs.h
 *
 * This matters: a register offset attached to the wrong regmap writes a
 * different block entirely.
 *
 * The vendor DTS is the third check. archive/versions/v3-lindroid3/fork_t618/
 * .../dts/sprd/sharkl5Pro.dtsi, on every one of the three csi nodes:
 *     sprd,aon-apb-syscon          = <&aon_apb_regs>;
 *     sprd,cam-ahb-syscon          = <&mm_ahb_regs>;
 *     sprd,anlg_phy_g10_controller = <&anlg_phy_g10_regs>;
 *
 * Naming: the CSI driver inserts an "RF_" path segment that the U-Boot headers
 * omit (REG_MM_AHB_RF_AHB_EB vs REG_MM_AHB_AHB_EB; ..._G10_RF_ANALOG_... vs
 * ..._G10_ANALOG_...; REG_AON_APB_RF_CGM_CLK_TOP_REG1 vs
 * REG_AON_APB_CGM_CLK_TOP_REG1). They name the same registers. The driver's
 * spelling is used here because the driver is the consumer, with the U-Boot
 * name quoted alongside so the pairing stays auditable.
 * ------------------------------------------------------------------ */

/*
 * sprd,cam-ahb-syscon  --  MM_AHB block.
 * mm_ahb.h: AHB_EB 0x0000, AHB_RST 0x0004, MIPI_CSI_SEL_CTRL 0x0030.
 * Blob agreement: 0x04 (csi_ahb_reset, cleared then set around a 4.3 us
 * delay) and 0x30 (phy_csi_path_cfg / cphy path select).
 * U-Boot equivalents: REG_MM_AHB_AHB_EB, REG_MM_AHB_AHB_RST,
 * REG_MM_AHB_MIPI_CSI_SEL_CTRL.
 */
#define REG_MM_AHB_RF_AHB_EB		0x0000
#define REG_MM_AHB_RF_AHB_RST		0x0004
#define REG_MM_AHB_RF_MIPI_CSI_SEL_CTRL	0x0030

/*
 * sprd,anlg_phy_g10_controller  --  ANLG_PHY_G10 block.
 * anlg_phy_g10.h states the absolute addresses in its "bits definitions for"
 * comments -- 0x323F0014, 0x323F0038, 0x323F007C, 0x323F00B4 -- i.e. block
 * base 0x323F0000 plus these offsets. Our own tree agrees: ums512.dtsi
 * declares anlg_phy_g10_regs as syscon@323f0000. All four were also read out
 * of the blob independently (0x14 is new relative to the sweep, which had
 * grouped 0x30/0x7c/0xb4 without their block).
 */
#define REG_ANLG_PHY_G10_RF_ANALOG_MIPI_CSI_2LANE_MIPI_PHY_BIST_TEST	0x0014
#define REG_ANLG_PHY_G10_RF_ANALOG_MIPI_CSI_2P2LANE_CTRL_CSI_2P2L	0x0038
#define REG_ANLG_PHY_G10_RF_ANALOG_MIPI_CSI_2P2LANE_CSI_2P2L_TEST_DB	0x007C
#define REG_ANLG_PHY_G10_RF_ANALOG_MIPI_CSI_COMBO_CSI_4L_BIST_TEST	0x00B4

/*
 * sprd,aon-apb-syscon  --  AON_APB block. Used only by csi_mipi_clk_enable()
 * and csi_mipi_clk_disable(). sprd,sharkl5pro_aon_apb-regs.h:
 * REG_AON_APB_CGM_CLK_TOP_REG1 = 0x013C; blob sweep had 0x13c.
 */
#define REG_AON_APB_RF_CGM_CLK_TOP_REG1	0x013C

/*
 * RETIRED by this cross-check, and why, so they are not reintroduced:
 *
 *  - AON_APB_REG_A..F and CAM_AHB_CSI_RESET: placeholders written before the
 *    regmap was known; superseded by the named macros above.
 *  - 0x7000. Present in NEITHER block: anlg_phy_g10.h's block stops at 0x00C0
 *    and aon_apb.h's stops at 0x0A08, and no regmap_update_bits() site in the
 *    source uses it. It was a relocation-adjacent artifact picked up by the
 *    disassembly sweep, not a register. Same family as the "relocation lines
 *    must be excluded from the instruction map" trap.
 */

/*
 * Vendor spellings required by csi_api.c.
 *
 * The source writes regmap_update_bits_base() against these exact names. The
 * offsets are the ones measured at the corresponding call sites in
 * sprd_sensor.ko, with the mask used at each:
 *
 *   0x0038  mask 0x10     REG_AON_APB_CSI_2L_PHY_CTRL
 *   0x007c  mask 0x01     REG_AON_APB_CSI_2P2L_RCTL
 *   0x013c  mask 0x8000   REG_AON_APB_CSI_2P2L_M_PHY_CTRL
 *   0x7000  mask 0x30     REG_AON_APB_CSI_2P2L_DBG_TRIMBG
 *
 * These deliberately coexist with the DT-derived REG_MM_AHB_* and
 * REG_ANLG_PHY_G10_* spellings above. Two names may land on the same offset:
 * the DT decides which syscon a given call reaches, and the source decides
 * which spelling it asks for. Collapsing them would lose information.
 */
#define REG_AON_APB_CSI_2L_PHY_CTRL		0x0038
#define REG_AON_APB_CSI_2P2L_RCTL		0x007c
#define REG_AON_APB_CSI_2P2L_M_PHY_CTRL		0x013c
#define REG_AON_APB_CSI_2P2L_DBG_PHY_CTRL	0x7000

#endif /* _DT_BINDINGS_SOC_SPRD_SHARKL5PRO_REGS_H */