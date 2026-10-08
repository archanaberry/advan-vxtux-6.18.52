/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Driver header file for pin controller driver
 * Copyright (C) 2017 Spreadtrum  - http://www.spreadtrum.com
 */

#ifndef __PINCTRL_SPRD_H__
#define __PINCTRL_SPRD_H__

struct platform_device;

#define NUM_OFFSET	(20)
#define TYPE_OFFSET	(16)
#define BIT_OFFSET	(8)
#define WIDTH_OFFSET	(4)

#define SPRD_PIN_INFO(num, type, offset, width, reg)	\
		(((num) & 0xFFF) << NUM_OFFSET |	\
		 ((type) & 0xF) << TYPE_OFFSET |	\
		 ((offset) & 0xFF) << BIT_OFFSET |	\
		 ((width) & 0xF) << WIDTH_OFFSET |	\
		 ((reg) & 0xF))

#define SPRD_PINCTRL_PIN(pin)	SPRD_PINCTRL_PIN_DATA(pin, #pin)

#define SPRD_PINCTRL_PIN_DATA(a, b)				\
	{							\
		.name = b,					\
		.num = (((a) >> NUM_OFFSET) & 0xfff),		\
		.type = (((a) >> TYPE_OFFSET) & 0xf),		\
		.bit_offset = (((a) >> BIT_OFFSET) & 0xff),	\
		.bit_width = ((a) >> WIDTH_OFFSET & 0xf),	\
		.reg = ((a) & 0xf)				\
	}

enum pin_type {
	GLOBAL_CTRL_PIN,
	COMMON_PIN,
	MISC_PIN,
};

struct sprd_pins_info {
	const char *name;
	unsigned int num;
	enum pin_type type;

	/* for global control pins configuration */
	unsigned long bit_offset;
	unsigned long bit_width;
	unsigned int reg;
};

int sprd_pinctrl_core_probe(struct platform_device *pdev,
			    struct sprd_pins_info *sprd_soc_pin_info,
			    int pins_cnt);
/*
 * VXTux (B45, 2026-09-27) — entry point ber-offset.
 *
 * Spreadtrum memakai offset register pin COMMON/MISC yang BERBEDA per SoC:
 * SC9860 = 0x20/0x4020 (konstanta mainline), Sharkl5Pro/UMS512 = 0x34/0x434.
 * Bukti UMS512 dari blob pinctrl-sprd-sharkl5Pro.ko: disasm fungsi
 * sprd_pinctrl_probe melempar w3=0x34 dan w4=0x434 ke core, dan tabelnya
 * 393 entri (0x3d68 byte / 40 byte per entri). Rincian di kepala
 * pinctrl-sprd-sharkl5pro.c.
 *
 * sprd_pinctrl_core_probe() (3 argumen) DIPERTAHANKAN UTUH sebagai pembungkus
 * ber-konstanta SC9860, jadi perilaku pemakai upstream tidak berubah sedikit
 * pun; driver SoC lain memakai entry point ini.
 */
int sprd_pinctrl_core_probe_offsets(struct platform_device *pdev,
				    struct sprd_pins_info *sprd_soc_pin_info,
				    int pins_cnt, u32 common_pin_offset,
				    u32 misc_pin_offset);
void sprd_pinctrl_remove(struct platform_device *pdev);
void sprd_pinctrl_shutdown(struct platform_device *pdev);

#endif /* __PINCTRL_SPRD_H__ */
