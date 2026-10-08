// SPDX-License-Identifier: GPL-2.0
/*
 * Driver for GalaxyCore gc8034 image sensor
 *
 * Copyright 2024 MediaTek
 *
 * Zhi Mao <zhi.mao@mediatek.com>
 */
#include <linux/array_size.h>
#include <linux/bits.h>
#include <linux/clk.h>
#include <linux/container_of.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/gpio/consumer.h>
#include <linux/math64.h>
#include <linux/mod_devicetable.h>
#include <linux/pm_runtime.h>
#include <linux/property.h>
#include <linux/regulator/consumer.h>
#include <linux/types.h>
#include <linux/units.h>

#include <media/v4l2-cci.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-subdev.h>

#define GC8034_REG_TEST_PATTERN_EN CCI_REG8(0x008c)
#define GC8034_REG_TEST_PATTERN_IDX CCI_REG8(0x008d)
#define GC8034_TEST_PATTERN_EN 0x01

#define GC8034_STREAMING_REG CCI_REG8(0x0100)

#define GC8034_FLIP_REG CCI_REG8(0x0101)
#define GC8034_FLIP_H_MASK BIT(0)
#define GC8034_FLIP_V_MASK BIT(1)

#define GC8034_EXP_REG CCI_REG16(0x0202)
#define GC8034_EXP_MARGIN 16
#define GC8034_EXP_MIN 4
#define GC8034_EXP_STEP 1

#define GC8034_AGAIN_REG CCI_REG16(0x0204)
#define GC8034_AGAIN_MIN 1024
#define GC8034_AGAIN_MAX (1024 * 16)
#define GC8034_AGAIN_STEP 1

#define GC8034_FRAME_LENGTH_REG CCI_REG16(0x0340)
#define GC8034_VTS_MAX 0xffff

#define GC8034_REG_CHIP_ID CCI_REG16(0x03f0)
#define GC8034_CHIP_ID 0x8034

#define GC8034_NATIVE_WIDTH 3264
#define GC8034_NATIVE_HEIGHT 2448

#define GC8034_DEFAULT_CLK_FREQ (24 * HZ_PER_MHZ)
#define GC8034_MBUS_CODE MEDIA_BUS_FMT_SGRBG10_1X10
#define GC8034_DATA_LANES 2
#define GC8034_RGB_DEPTH 10
#define GC8034_SLEEP_US  (2 * USEC_PER_MSEC)

static const char *const gc8034_test_pattern_menu[] = {
	"No Pattern",  "Fade_to_gray_Color Bar", "Color Bar",
	"PN9",	       "Horizontal_gradient",    "Checkboard Pattern",
	"Slant",       "Resolution",	         "Solid Black",
	"Solid White",
};

static const s64 gc8034_link_freq_menu_items[] = {
	(448 * HZ_PER_MHZ),
	(224 * HZ_PER_MHZ),
};

static const char *const gc8034_supply_name[] = {
	"avdd",
	"dvdd",
	"dovdd",
};

struct gc8034 {
	struct device *dev;
	struct v4l2_subdev sd;
	struct media_pad pad;

	struct clk *xclk;
	struct regulator_bulk_data supplies[ARRAY_SIZE(gc8034_supply_name)];
	struct gpio_desc *reset_gpio;

	struct v4l2_ctrl_handler ctrls;
	struct v4l2_ctrl *pixel_rate;
	struct v4l2_ctrl *link_freq;
	struct v4l2_ctrl *exposure;
	struct v4l2_ctrl *vblank;
	struct v4l2_ctrl *hblank;
	struct v4l2_ctrl *hflip;
	struct v4l2_ctrl *vflip;

	struct regmap *regmap;
	unsigned long link_freq_bitmap;

	/* True if the device has been identified */
	bool identified;
	const struct gc8034_mode *cur_mode;
};

struct gc8034_reg_list {
	u32 num_of_regs;
	const struct cci_reg_sequence *regs;
};

static const struct cci_reg_sequence mode_3264x2448[] = {
	/* system */
	{ CCI_REG8(0x0135), 0x01 },
	{ CCI_REG8(0x0084), 0x21 },
	{ CCI_REG8(0x0d05), 0xcc },
	{ CCI_REG8(0x0218), 0x00 },
	{ CCI_REG8(0x005e), 0x48 },
	{ CCI_REG8(0x0d06), 0x01 },
	{ CCI_REG8(0x0007), 0x16 },
	{ CCI_REG8(0x0101), 0x00 },

	/* analog */
	{ CCI_REG8(0x0342), 0x07 },
	{ CCI_REG8(0x0343), 0x28 },
	{ CCI_REG8(0x0220), 0x07 },
	{ CCI_REG8(0x0221), 0xd0 },
	{ CCI_REG8(0x0202), 0x07 },
	{ CCI_REG8(0x0203), 0x32 },
	{ CCI_REG8(0x0340), 0x07 },
	{ CCI_REG8(0x0341), 0xf0 },
	{ CCI_REG8(0x0219), 0x00 },
	{ CCI_REG8(0x0346), 0x00 },
	{ CCI_REG8(0x0347), 0x04 },
	{ CCI_REG8(0x0d14), 0x00 },
	{ CCI_REG8(0x0d13), 0x05 },
	{ CCI_REG8(0x0d16), 0x05 },
	{ CCI_REG8(0x0d15), 0x1d },
	{ CCI_REG8(0x00c0), 0x0a },
	{ CCI_REG8(0x00c1), 0x30 },
	{ CCI_REG8(0x034a), 0x07 },
	{ CCI_REG8(0x034b), 0xa8 },
	{ CCI_REG8(0x0e0a), 0x00 },
	{ CCI_REG8(0x0e0b), 0x00 },
	{ CCI_REG8(0x0e0e), 0x03 },
	{ CCI_REG8(0x0e0f), 0x00 },
	{ CCI_REG8(0x0e06), 0x0a },
	{ CCI_REG8(0x0e23), 0x15 },
	{ CCI_REG8(0x0e24), 0x15 },
	{ CCI_REG8(0x0e2a), 0x10 },
	{ CCI_REG8(0x0e2b), 0x10 },
	{ CCI_REG8(0x0e17), 0x49 },
	{ CCI_REG8(0x0e1b), 0x1c },
	{ CCI_REG8(0x0e3a), 0x36 },
	{ CCI_REG8(0x0d11), 0x84 },
	{ CCI_REG8(0x0e52), 0x14 },
	{ CCI_REG8(0x000b), 0x10 },
	{ CCI_REG8(0x0008), 0x08 },
	{ CCI_REG8(0x0223), 0x17 },
	{ CCI_REG8(0x0d27), 0x39 },
	{ CCI_REG8(0x0d22), 0x00 },
	{ CCI_REG8(0x03f6), 0x0d },
	{ CCI_REG8(0x0d04), 0x07 },
	{ CCI_REG8(0x03f3), 0x72 },
	{ CCI_REG8(0x03f4), 0xb8 },
	{ CCI_REG8(0x03f5), 0xbc },
	{ CCI_REG8(0x0d02), 0x73 },

	/* auto load start */
	{ CCI_REG8(0x00cb), 0x00 },

	/* OUT 3264*2448 */
	{ CCI_REG8(0x0350), 0x01 },
	{ CCI_REG8(0x0353), 0x00 },
	{ CCI_REG8(0x0354), 0x08 },
	{ CCI_REG16(0x034c), 3264 }, /* Width */
	{ CCI_REG8(0x021f), 0x14 },

	/* MIPI */
	{ CCI_REG8(0x0107), 0x05 },
	{ CCI_REG8(0x0117), 0x01 },
	{ CCI_REG8(0x0d81), 0x00 },
	{ CCI_REG8(0x0d84), 0x0c },
	{ CCI_REG8(0x0d85), 0xa8 },
	{ CCI_REG8(0x0d86), 0x06 },
	{ CCI_REG8(0x0d87), 0x55 },
	{ CCI_REG8(0x0db3), 0x06 },
	{ CCI_REG8(0x0db4), 0x08 },
	{ CCI_REG8(0x0db5), 0x1e },
	{ CCI_REG8(0x0db6), 0x02 },
	{ CCI_REG8(0x0db8), 0x12 },
	{ CCI_REG8(0x0db9), 0x0a },
	{ CCI_REG8(0x0d93), 0x06 },
	{ CCI_REG8(0x0d94), 0x09 },
	{ CCI_REG8(0x0d95), 0x0d },
	{ CCI_REG8(0x0d99), 0x0b },
	{ CCI_REG8(0x0084), 0x01 },
	{ CCI_REG8(0x0110), 0x01 },
};

static const struct cci_reg_sequence mode_1280x720[] = {
	/* system */
	{ CCI_REG8(0x0135), 0x05 },
	{ CCI_REG8(0x0084), 0x21 },
	{ CCI_REG8(0x0d05), 0xcc },
	{ CCI_REG8(0x0218), 0x80 },
	{ CCI_REG8(0x005e), 0x49 },
	{ CCI_REG8(0x0d06), 0x81 },
	{ CCI_REG8(0x0007), 0x16 },
	{ CCI_REG8(0x0101), 0x00 },

	/* analog */
	{ CCI_REG8(0x0342), 0x07 },
	{ CCI_REG8(0x0343), 0x10 },
	{ CCI_REG8(0x0220), 0x07 },
	{ CCI_REG8(0x0221), 0xd0 },
	{ CCI_REG8(0x0202), 0x03 },
	{ CCI_REG8(0x0203), 0x32 },
	{ CCI_REG8(0x0340), 0x04 },
	{ CCI_REG8(0x0341), 0x08 },
	{ CCI_REG8(0x0219), 0x00 },
	{ CCI_REG8(0x0346), 0x01 },
	{ CCI_REG8(0x0347), 0x00 },
	{ CCI_REG8(0x0d14), 0x00 },
	{ CCI_REG8(0x0d13), 0x05 },
	{ CCI_REG8(0x0d16), 0x05 },
	{ CCI_REG8(0x0d15), 0x1d },
	{ CCI_REG8(0x00c0), 0x0a },
	{ CCI_REG8(0x00c1), 0x30 },
	{ CCI_REG8(0x034a), 0x05 },
	{ CCI_REG8(0x034b), 0xb0 },
	{ CCI_REG8(0x0e0a), 0x00 },
	{ CCI_REG8(0x0e0b), 0x00 },
	{ CCI_REG8(0x0e0e), 0x03 },
	{ CCI_REG8(0x0e0f), 0x00 },
	{ CCI_REG8(0x0e06), 0x0a },
	{ CCI_REG8(0x0e23), 0x15 },
	{ CCI_REG8(0x0e24), 0x15 },
	{ CCI_REG8(0x0e2a), 0x10 },
	{ CCI_REG8(0x0e2b), 0x10 },
	{ CCI_REG8(0x0e17), 0x49 },
	{ CCI_REG8(0x0e1b), 0x1c },
	{ CCI_REG8(0x0e3a), 0x36 },
	{ CCI_REG8(0x0d11), 0x84 },
	{ CCI_REG8(0x0e52), 0x14 },
	{ CCI_REG8(0x000b), 0x0e },
	{ CCI_REG8(0x0008), 0x03 },
	{ CCI_REG8(0x0223), 0x16 },
	{ CCI_REG8(0x0d27), 0x39 },
	{ CCI_REG8(0x0d22), 0x00 },
	{ CCI_REG8(0x03f6), 0x0d },
	{ CCI_REG8(0x0d04), 0x07 },
	{ CCI_REG8(0x03f3), 0x72 },
	{ CCI_REG8(0x03f4), 0xb8 },
	{ CCI_REG8(0x03f5), 0xbc },
	{ CCI_REG8(0x0d02), 0x73 },

	/* auto load start */
	{ CCI_REG8(0x00cb), 0xfc },

	/* OUT 1280x720 */
	{ CCI_REG8(0x0350), 0x01 },
	{ CCI_REG8(0x0353), 0x00 },
	{ CCI_REG8(0x0354), 0x0c },
	{ CCI_REG16(0x034c), 1280 }, /* Width */
	{ CCI_REG8(0x021f), 0x14 },

	/* MIPI */
	{ CCI_REG8(0x0107), 0x05 },
	{ CCI_REG8(0x0117), 0x01 },
	{ CCI_REG8(0x0d81), 0x00 },
	{ CCI_REG8(0x0d84), 0x06 },
	{ CCI_REG8(0x0d85), 0x40 },
	{ CCI_REG8(0x0d86), 0x03 },
	{ CCI_REG8(0x0d87), 0x21 },
	{ CCI_REG8(0x0db3), 0x03 },
	{ CCI_REG8(0x0db4), 0x04 },
	{ CCI_REG8(0x0db5), 0x0d },
	{ CCI_REG8(0x0db6), 0x01 },
	{ CCI_REG8(0x0db8), 0x04 },
	{ CCI_REG8(0x0db9), 0x06 },
	{ CCI_REG8(0x0d93), 0x03 },
	{ CCI_REG8(0x0d94), 0x04 },
	{ CCI_REG8(0x0d95), 0x05 },
	{ CCI_REG8(0x0d99), 0x06 },
	{ CCI_REG8(0x0084), 0x01 },
	{ CCI_REG8(0x0110), 0x01 },
};

static const struct cci_reg_sequence mode_table_common[] = {
	{ GC8034_STREAMING_REG, 0x00 },
	/* system */
	{ CCI_REG8(0x0315), 0xd4 },
	{ CCI_REG8(0x0d06), 0x01 },
	{ CCI_REG8(0x0a70), 0x80 },
	{ CCI_REG8(0x031a), 0x00 },
	{ CCI_REG8(0x0314), 0x00 },
	{ CCI_REG8(0x0130), 0x08 },
	{ CCI_REG8(0x0132), 0x01 },
	{ CCI_REG8(0x0136), 0x38 },
	{ CCI_REG8(0x0137), 0x03 },
	{ CCI_REG8(0x0134), 0x5b },
	{ CCI_REG8(0x031c), 0xe0 },
	{ CCI_REG8(0x0d82), 0x14 },
	{ CCI_REG8(0x0dd1), 0x56 },
	{ CCI_REG8(0x0af4), 0x01 },
	{ CCI_REG8(0x0002), 0x10 },
	{ CCI_REG8(0x00c3), 0x34 },
	{ CCI_REG8(0x00c4), 0x00 },
	{ CCI_REG8(0x00c5), 0x01 },
	{ CCI_REG8(0x0af6), 0x00 },
	{ CCI_REG8(0x0ba0), 0x17 },
	{ CCI_REG8(0x0ba1), 0x00 },
	{ CCI_REG8(0x0ba2), 0x00 },
	{ CCI_REG8(0x0ba3), 0x00 },
	{ CCI_REG8(0x0ba4), 0x03 },
	{ CCI_REG8(0x0ba5), 0x00 },
	{ CCI_REG8(0x0ba6), 0x00 },
	{ CCI_REG8(0x0ba7), 0x00 },
	{ CCI_REG8(0x0ba8), 0x40 },
	{ CCI_REG8(0x0ba9), 0x00 },
	{ CCI_REG8(0x0baa), 0x00 },
	{ CCI_REG8(0x0bab), 0x00 },
	{ CCI_REG8(0x0bac), 0x40 },
	{ CCI_REG8(0x0bad), 0x00 },
	{ CCI_REG8(0x0bae), 0x00 },
	{ CCI_REG8(0x0baf), 0x00 },
	{ CCI_REG8(0x0bb0), 0x02 },
	{ CCI_REG8(0x0bb1), 0x00 },
	{ CCI_REG8(0x0bb2), 0x00 },
	{ CCI_REG8(0x0bb3), 0x00 },
	{ CCI_REG8(0x0bb8), 0x02 },
	{ CCI_REG8(0x0bb9), 0x00 },
	{ CCI_REG8(0x0bba), 0x00 },
	{ CCI_REG8(0x0bbb), 0x00 },
	{ CCI_REG8(0x0a70), 0x80 },
	{ CCI_REG8(0x0a71), 0x00 },
	{ CCI_REG8(0x0a72), 0x00 },
	{ CCI_REG8(0x0a66), 0x00 },
	{ CCI_REG8(0x0a67), 0x80 },
	{ CCI_REG8(0x0a4d), 0x4e },
	{ CCI_REG8(0x0a50), 0x00 },
	{ CCI_REG8(0x0a4f), 0x0c },
	{ CCI_REG8(0x0a66), 0x00 },
	{ CCI_REG8(0x00ca), 0x00 },
	{ CCI_REG8(0x00cc), 0x00 },
	{ CCI_REG8(0x00cd), 0x00 },
	{ CCI_REG8(0x0aa1), 0x00 },
	{ CCI_REG8(0x0aa2), 0xe0 },
	{ CCI_REG8(0x0aa3), 0x00 },
	{ CCI_REG8(0x0aa4), 0x40 },
	{ CCI_REG8(0x0a90), 0x03 },
	{ CCI_REG8(0x0a91), 0x0e },
	{ CCI_REG8(0x0a94), 0x80 },
	{ CCI_REG8(0x0af6), 0x20 },
	{ CCI_REG8(0x0b00), 0x91 },
	{ CCI_REG8(0x0b01), 0x17 },
	{ CCI_REG8(0x0b02), 0x01 },
	{ CCI_REG8(0x0b03), 0x00 },
	{ CCI_REG8(0x0b04), 0x01 },
	{ CCI_REG8(0x0b05), 0x17 },
	{ CCI_REG8(0x0b06), 0x01 },
	{ CCI_REG8(0x0b07), 0x00 },
	{ CCI_REG8(0x0ae9), 0x01 },
	{ CCI_REG8(0x0aea), 0x02 },
	{ CCI_REG8(0x0ae8), 0x53 },
	{ CCI_REG8(0x0ae8), 0x43 },
	{ CCI_REG8(0x0af6), 0x30 },
	{ CCI_REG8(0x0b00), 0x08 },
	{ CCI_REG8(0x0b01), 0x0f },
	{ CCI_REG8(0x0b02), 0x00 },
	{ CCI_REG8(0x0b04), 0x1c },
	{ CCI_REG8(0x0b05), 0x24 },
	{ CCI_REG8(0x0b06), 0x00 },
	{ CCI_REG8(0x0b08), 0x30 },
	{ CCI_REG8(0x0b09), 0x40 },
	{ CCI_REG8(0x0b0a), 0x00 },
	{ CCI_REG8(0x0b0c), 0x0e },
	{ CCI_REG8(0x0b0d), 0x2a },
	{ CCI_REG8(0x0b0e), 0x00 },
	{ CCI_REG8(0x0b10), 0x0e },
	{ CCI_REG8(0x0b11), 0x2b },
	{ CCI_REG8(0x0b12), 0x00 },
	{ CCI_REG8(0x0b14), 0x0e },
	{ CCI_REG8(0x0b15), 0x23 },
	{ CCI_REG8(0x0b16), 0x00 },
	{ CCI_REG8(0x0b18), 0x0e },
	{ CCI_REG8(0x0b19), 0x24 },
	{ CCI_REG8(0x0b1a), 0x00 },
	{ CCI_REG8(0x0b1c), 0x0c },
	{ CCI_REG8(0x0b1d), 0x0c },
	{ CCI_REG8(0x0b1e), 0x00 },
	{ CCI_REG8(0x0b20), 0x03 },
	{ CCI_REG8(0x0b21), 0x03 },
	{ CCI_REG8(0x0b22), 0x00 },
	{ CCI_REG8(0x0b24), 0x0e },
	{ CCI_REG8(0x0b25), 0x0e },
	{ CCI_REG8(0x0b26), 0x00 },
	{ CCI_REG8(0x0b28), 0x03 },
	{ CCI_REG8(0x0b29), 0x03 },
	{ CCI_REG8(0x0b2a), 0x00 },
	{ CCI_REG8(0x0b2c), 0x12 },
	{ CCI_REG8(0x0b2d), 0x12 },
	{ CCI_REG8(0x0b2e), 0x00 },
	{ CCI_REG8(0x0b30), 0x08 },
	{ CCI_REG8(0x0b31), 0x08 },
	{ CCI_REG8(0x0b32), 0x00 },
	{ CCI_REG8(0x0b34), 0x14 },
	{ CCI_REG8(0x0b35), 0x14 },
	{ CCI_REG8(0x0b36), 0x00 },
	{ CCI_REG8(0x0b38), 0x10 },
	{ CCI_REG8(0x0b39), 0x10 },
	{ CCI_REG8(0x0b3a), 0x00 },
	{ CCI_REG8(0x0b3c), 0x16 },
	{ CCI_REG8(0x0b3d), 0x16 },
	{ CCI_REG8(0x0b3e), 0x00 },
	{ CCI_REG8(0x0b40), 0x10 },
	{ CCI_REG8(0x0b41), 0x10 },
	{ CCI_REG8(0x0b42), 0x00 },
	{ CCI_REG8(0x0b44), 0x19 },
	{ CCI_REG8(0x0b45), 0x19 },
	{ CCI_REG8(0x0b46), 0x00 },
	{ CCI_REG8(0x0b48), 0x16 },
	{ CCI_REG8(0x0b49), 0x16 },
	{ CCI_REG8(0x0b4a), 0x00 },
	{ CCI_REG8(0x0b4c), 0x19 },
	{ CCI_REG8(0x0b4d), 0x19 },
	{ CCI_REG8(0x0b4e), 0x00 },
	{ CCI_REG8(0x0b50), 0x16 },
	{ CCI_REG8(0x0b51), 0x16 },
	{ CCI_REG8(0x0b52), 0x00 },
	{ CCI_REG8(0x0b80), 0x01 },
	{ CCI_REG8(0x0b81), 0x00 },
	{ CCI_REG8(0x0b82), 0x00 },
	{ CCI_REG8(0x0b84), 0x00 },
	{ CCI_REG8(0x0b85), 0x00 },
	{ CCI_REG8(0x0b86), 0x00 },
	{ CCI_REG8(0x0b88), 0x01 },
	{ CCI_REG8(0x0b89), 0x6a },
	{ CCI_REG8(0x0b8a), 0x00 },
	{ CCI_REG8(0x0b8c), 0x00 },
	{ CCI_REG8(0x0b8d), 0x01 },
	{ CCI_REG8(0x0b8e), 0x00 },
	{ CCI_REG8(0x0b90), 0x01 },
	{ CCI_REG8(0x0b91), 0xf6 },
	{ CCI_REG8(0x0b92), 0x00 },
	{ CCI_REG8(0x0b94), 0x00 },
	{ CCI_REG8(0x0b95), 0x02 },
	{ CCI_REG8(0x0b96), 0x00 },
	{ CCI_REG8(0x0b98), 0x02 },
	{ CCI_REG8(0x0b99), 0xc4 },
	{ CCI_REG8(0x0b9a), 0x00 },
	{ CCI_REG8(0x0b9c), 0x00 },
	{ CCI_REG8(0x0b9d), 0x03 },
	{ CCI_REG8(0x0b9e), 0x00 },
	{ CCI_REG8(0x0ba0), 0x03 },
	{ CCI_REG8(0x0ba1), 0xd8 },
	{ CCI_REG8(0x0ba2), 0x00 },
	{ CCI_REG8(0x0ba4), 0x00 },
	{ CCI_REG8(0x0ba5), 0x04 },
	{ CCI_REG8(0x0ba6), 0x00 },
	{ CCI_REG8(0x0ba8), 0x05 },
	{ CCI_REG8(0x0ba9), 0x4d },
	{ CCI_REG8(0x0baa), 0x00 },
	{ CCI_REG8(0x0bac), 0x00 },
	{ CCI_REG8(0x0bad), 0x05 },
	{ CCI_REG8(0x0bae), 0x00 },
	{ CCI_REG8(0x0bb0), 0x07 },
	{ CCI_REG8(0x0bb1), 0x3e },
	{ CCI_REG8(0x0bb2), 0x00 },
	{ CCI_REG8(0x0bb4), 0x00 },
	{ CCI_REG8(0x0bb5), 0x06 },
	{ CCI_REG8(0x0bb6), 0x00 },
	{ CCI_REG8(0x0bb8), 0x0a },
	{ CCI_REG8(0x0bb9), 0x1a },
	{ CCI_REG8(0x0bba), 0x00 },
	{ CCI_REG8(0x0bbc), 0x09 },
	{ CCI_REG8(0x0bbd), 0x36 },
	{ CCI_REG8(0x0bbe), 0x00 },
	{ CCI_REG8(0x0bc0), 0x0e },
	{ CCI_REG8(0x0bc1), 0x66 },
	{ CCI_REG8(0x0bc2), 0x00 },
	{ CCI_REG8(0x0bc4), 0x10 },
	{ CCI_REG8(0x0bc5), 0x06 },
	{ CCI_REG8(0x0bc6), 0x00 },
	{ CCI_REG8(0x02c1), 0xe0 },
	{ CCI_REG8(0x0207), 0x04 },
	{ CCI_REG8(0x02c2), 0x10 },
	{ CCI_REG8(0x02c3), 0x74 },
	{ CCI_REG8(0x02c5), 0x09 },
	{ CCI_REG8(0x02c1), 0xe0 },
	{ CCI_REG8(0x0207), 0x04 },
	{ CCI_REG8(0x02c2), 0x10 },
	{ CCI_REG8(0x02c5), 0x09 },
	{ CCI_REG8(0x02c1), 0xe0 },
	{ CCI_REG8(0x0207), 0x04 },
	{ CCI_REG8(0x02c2), 0x10 },
	{ CCI_REG8(0x02c5), 0x09 },
	{ CCI_REG8(0x0aa1), 0x15 },
	{ CCI_REG8(0x0aa2), 0x50 },
	{ CCI_REG8(0x0aa3), 0x00 },
	{ CCI_REG8(0x0aa4), 0x09 },
	{ CCI_REG8(0x0a90), 0x25 },
	{ CCI_REG8(0x0a91), 0x0e },
	{ CCI_REG8(0x0a94), 0x80 },

	/* ISP */
	{ CCI_REG8(0x0050), 0x00 },
	{ CCI_REG8(0x0089), 0x83 },
	{ CCI_REG8(0x005a), 0x40 },
	{ CCI_REG8(0x00c3), 0x35 },
	{ CCI_REG8(0x00c4), 0x80 },
	{ CCI_REG8(0x0080), 0x10 },
	{ CCI_REG8(0x0040), 0x12 },
	{ CCI_REG8(0x0053), 0x0a },
	{ CCI_REG8(0x0054), 0x44 },
	{ CCI_REG8(0x0055), 0x32 },
	{ CCI_REG8(0x0058), 0x89 },
	{ CCI_REG8(0x004a), 0x03 },
	{ CCI_REG8(0x0048), 0xf0 },
	{ CCI_REG8(0x0049), 0x0f },
	{ CCI_REG8(0x0041), 0x20 },
	{ CCI_REG8(0x0043), 0x0a },
	{ CCI_REG8(0x009d), 0x08 },
	{ CCI_REG8(0x0236), 0x40 },
	{ CCI_REG8(0x0204), 0x04 },
	{ CCI_REG8(0x0205), 0x00 },
	{ CCI_REG8(0x02b3), 0x00 },
	{ CCI_REG8(0x02b4), 0x00 },
	{ CCI_REG8(0x009e), 0x01 },
	{ CCI_REG8(0x009f), 0x94 },

	/* auto load REG */
	{ CCI_REG8(0x0aa1), 0x10 },
	{ CCI_REG8(0x0aa2), 0xf8 },
	{ CCI_REG8(0x0aa3), 0x00 },
	{ CCI_REG8(0x0aa4), 0x1f },
	{ CCI_REG8(0x0a90), 0x11 },
	{ CCI_REG8(0x0a91), 0x0e },
	{ CCI_REG8(0x0a94), 0x80 },
	{ CCI_REG8(0x03fe), 0x00 },
	{ CCI_REG8(0x0a90), 0x00 },
	{ CCI_REG8(0x0a70), 0x00 },
	{ CCI_REG8(0x0a67), 0x00 },
	{ CCI_REG8(0x0af4), 0x29 },

	/* DPHY */
	{ CCI_REG8(0x0d80), 0x07 },
	{ CCI_REG8(0x0dd3), 0x18 },

	/* CISCTL_Reset */
	{ CCI_REG8(0x031c), 0x80 },
	{ CCI_REG8(0x03fe), 0x30 },
	{ CCI_REG8(0x0d17), 0x06 },
	{ CCI_REG8(0x03fe), 0x00 },
	{ CCI_REG8(0x0d17), 0x00 },
	{ CCI_REG8(0x031c), 0x93 },
	{ CCI_REG8(0x03fe), 0x00 },
	{ CCI_REG8(0x031c), 0x80 },
	{ CCI_REG8(0x03fe), 0x30 },
	{ CCI_REG8(0x0d17), 0x06 },
	{ CCI_REG8(0x03fe), 0x00 },
	{ CCI_REG8(0x0d17), 0x00 },
	{ CCI_REG8(0x031c), 0x93 },
};

struct gc8034_mode {
	u32 width;
	u32 height;
	const struct gc8034_reg_list reg_list;

	u32 hts; /* Horizontal timining size */
	u32 vts_def; /* Default vertical timining size */
	u32 vts_min; /* Min vertical timining size */
};

/* Declare modes in order, from biggest to smallest height. */
static const struct gc8034_mode gc8034_modes[] = {
	{
		/* 3264*2448@30fps */
		.width = GC8034_NATIVE_WIDTH,
		.height = GC8034_NATIVE_HEIGHT,
		.reg_list = {
			.num_of_regs = ARRAY_SIZE(mode_3264x2448),
			.regs = mode_3264x2448,
		},
		.hts = 3664,
		.vts_def = 2032,
		.vts_min = 2032,
	},
	{
		/* 1280*720@60fps */
		.width = 1280,
		.height = 720,
		.reg_list = {
			.num_of_regs = ARRAY_SIZE(mode_1280x720),
			.regs = mode_1280x720,
		},
		.hts = 3616,
		.vts_def = 1032,
		.vts_min = 1032,
	},
};

static inline struct gc8034 *to_gc8034(struct v4l2_subdev *sd)
{
	return container_of(sd, struct gc8034, sd);
}

static int gc8034_power_on(struct device *dev)
{
	struct v4l2_subdev *sd = dev_get_drvdata(dev);
	struct gc8034 *gc8034 = to_gc8034(sd);
	int ret;

	ret = regulator_bulk_enable(ARRAY_SIZE(gc8034_supply_name),
				    gc8034->supplies);
	if (ret < 0) {
		dev_err(gc8034->dev, "failed to enable regulators: %d\n", ret);
		return ret;
	}

	ret = clk_prepare_enable(gc8034->xclk);
	if (ret < 0) {
		regulator_bulk_disable(ARRAY_SIZE(gc8034_supply_name),
				       gc8034->supplies);
		dev_err(gc8034->dev, "clk prepare enable failed\n");
		return ret;
	}

	fsleep(GC8034_SLEEP_US);

	gpiod_set_value_cansleep(gc8034->reset_gpio, 0);
	fsleep(GC8034_SLEEP_US);

	return 0;
}

static int gc8034_power_off(struct device *dev)
{
	struct v4l2_subdev *sd = dev_get_drvdata(dev);
	struct gc8034 *gc8034 = to_gc8034(sd);

	clk_disable_unprepare(gc8034->xclk);
	gpiod_set_value_cansleep(gc8034->reset_gpio, 1);
	regulator_bulk_disable(ARRAY_SIZE(gc8034_supply_name),
			       gc8034->supplies);

	return 0;
}

static int gc8034_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_state *sd_state,
				 struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->index > 0)
		return -EINVAL;

	code->code = GC8034_MBUS_CODE;

	return 0;
}

static int gc8034_enum_frame_size(struct v4l2_subdev *subdev,
				  struct v4l2_subdev_state *sd_state,
				  struct v4l2_subdev_frame_size_enum *fse)
{
	if (fse->code != GC8034_MBUS_CODE)
		return -EINVAL;

	if (fse->index >= ARRAY_SIZE(gc8034_modes))
		return -EINVAL;

	fse->min_width = gc8034_modes[fse->index].width;
	fse->max_width = gc8034_modes[fse->index].width;
	fse->min_height = gc8034_modes[fse->index].height;
	fse->max_height = gc8034_modes[fse->index].height;

	return 0;
}

static int gc8034_update_cur_mode_controls(struct gc8034 *gc8034,
					   const struct gc8034_mode *mode)
{
	s64 exposure_max, h_blank;
	int ret;

	ret = __v4l2_ctrl_modify_range(gc8034->vblank,
				       mode->vts_min - mode->height,
				       GC8034_VTS_MAX - mode->height, 1,
				       mode->vts_def - mode->height);
	if (ret) {
		dev_err(gc8034->dev, "VB ctrl range update failed\n");
		return ret;
	}

	h_blank = mode->hts - mode->width;
	ret = __v4l2_ctrl_modify_range(gc8034->hblank, h_blank, h_blank, 1,
				       h_blank);
	if (ret) {
		dev_err(gc8034->dev, "HB ctrl range update failed\n");
		return ret;
	}

	exposure_max = mode->vts_def - GC8034_EXP_MARGIN;
	ret = __v4l2_ctrl_modify_range(gc8034->exposure, GC8034_EXP_MIN,
				       exposure_max, GC8034_EXP_STEP,
				       exposure_max);
	if (ret) {
		dev_err(gc8034->dev, "exposure ctrl range update failed\n");
		return ret;
	}

	return 0;
}

static void gc8034_update_pad_format(struct gc8034 *gc08a3,
				     const struct gc8034_mode *mode,
				     struct v4l2_mbus_framefmt *fmt)
{
	fmt->width = mode->width;
	fmt->height = mode->height;
	fmt->code = GC8034_MBUS_CODE;
	fmt->field = V4L2_FIELD_NONE;
	fmt->colorspace = V4L2_COLORSPACE_RAW;
	fmt->ycbcr_enc = V4L2_MAP_YCBCR_ENC_DEFAULT(fmt->colorspace);
	fmt->quantization = V4L2_QUANTIZATION_FULL_RANGE;
	fmt->xfer_func = V4L2_XFER_FUNC_NONE;
}

static int gc8034_set_format(struct v4l2_subdev *sd,
			     struct v4l2_subdev_state *state,
			     struct v4l2_subdev_format *fmt)
{
	struct gc8034 *gc8034 = to_gc8034(sd);
	struct v4l2_mbus_framefmt *mbus_fmt;
	struct v4l2_rect *crop;
	const struct gc8034_mode *mode;

	mode = v4l2_find_nearest_size(gc8034_modes, ARRAY_SIZE(gc8034_modes),
				      width, height, fmt->format.width,
				      fmt->format.height);

	/* update crop info to subdev state */
	crop = v4l2_subdev_state_get_crop(state, 0);
	crop->width = mode->width;
	crop->height = mode->height;

	/* update fmt info to subdev state */
	gc8034_update_pad_format(gc8034, mode, &fmt->format);
	mbus_fmt = v4l2_subdev_state_get_format(state, 0);
	*mbus_fmt = fmt->format;

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY)
		return 0;
	gc8034->cur_mode = mode;
	gc8034_update_cur_mode_controls(gc8034, mode);

	return 0;
}

static int gc8034_get_selection(struct v4l2_subdev *sd,
				struct v4l2_subdev_state *state,
				struct v4l2_subdev_selection *sel)
{
	switch (sel->target) {
	case V4L2_SEL_TGT_CROP_DEFAULT:
	case V4L2_SEL_TGT_CROP:
		sel->r = *v4l2_subdev_state_get_crop(state, 0);
		break;
	case V4L2_SEL_TGT_CROP_BOUNDS:
		sel->r.top = 0;
		sel->r.left = 0;
		sel->r.width = GC8034_NATIVE_WIDTH;
		sel->r.height = GC8034_NATIVE_HEIGHT;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int gc8034_init_state(struct v4l2_subdev *sd,
			     struct v4l2_subdev_state *state)
{
	struct v4l2_subdev_format fmt = {
		.which = V4L2_SUBDEV_FORMAT_TRY,
		.pad = 0,
		.format = {
			.code = GC8034_MBUS_CODE,
			.width = gc8034_modes[0].width,
			.height = gc8034_modes[0].height,
		},
	};

	gc8034_set_format(sd, state, &fmt);

	return 0;
}

static int gc8034_set_ctrl_hflip(struct gc8034 *gc8034, u32 ctrl_val)
{
	int ret;
	u64 val;

	ret = cci_read(gc8034->regmap, GC8034_FLIP_REG, &val, NULL);
	if (ret) {
		dev_err(gc8034->dev, "read hflip register failed: %d\n", ret);
		return ret;
	}

	return cci_update_bits(gc8034->regmap, GC8034_FLIP_REG,
			       GC8034_FLIP_H_MASK,
			       ctrl_val ? GC8034_FLIP_H_MASK : 0, NULL);
}

static int gc8034_set_ctrl_vflip(struct gc8034 *gc8034, u32 ctrl_val)
{
	int ret;
	u64 val;

	ret = cci_read(gc8034->regmap, GC8034_FLIP_REG, &val, NULL);
	if (ret) {
		dev_err(gc8034->dev, "read vflip register failed: %d\n", ret);
		return ret;
	}

	return cci_update_bits(gc8034->regmap, GC8034_FLIP_REG,
			       GC8034_FLIP_V_MASK,
			       ctrl_val ? GC8034_FLIP_V_MASK : 0, NULL);
}

static int gc8034_test_pattern(struct gc8034 *gc8034, u32 pattern_menu)
{
	u32 pattern;
	int ret;

	if (pattern_menu) {
		switch (pattern_menu) {
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
			pattern = pattern_menu << 4;
			break;

		case 8:
			pattern = 0;
			break;

		case 9:
			pattern = 4;
			break;

		default:
			/* Set pattern to 0, it's a safe default. */
			pattern = 0;
			break;
		}

		ret = cci_write(gc8034->regmap, GC8034_REG_TEST_PATTERN_IDX,
				pattern, NULL);
		if (ret)
			return ret;

		return cci_write(gc8034->regmap, GC8034_REG_TEST_PATTERN_EN,
				 GC8034_TEST_PATTERN_EN, NULL);
	} else {
		return cci_write(gc8034->regmap, GC8034_REG_TEST_PATTERN_EN,
				 0x00, NULL);
	}
}

static int gc8034_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct gc8034 *gc8034 =
		container_of(ctrl->handler, struct gc8034, ctrls);
	int ret = 0;
	s64 exposure_max;
	struct v4l2_subdev_state *state;
	const struct v4l2_mbus_framefmt *format;

	state = v4l2_subdev_get_locked_active_state(&gc8034->sd);
	format = v4l2_subdev_state_get_format(state, 0);

	if (ctrl->id == V4L2_CID_VBLANK) {
		/* Update max exposure while meeting expected vblanking */
		exposure_max = format->height + ctrl->val - GC8034_EXP_MARGIN;
		__v4l2_ctrl_modify_range(gc8034->exposure,
					 gc8034->exposure->minimum,
					 exposure_max, gc8034->exposure->step,
					 exposure_max);
	}

	/*
	 * Applying V4L2 control value only happens
	 * when power is on for streaming.
	 */
	if (!pm_runtime_get_if_active(gc8034->dev))
		return 0;

	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE:
		ret = cci_write(gc8034->regmap, GC8034_EXP_REG,
				ctrl->val, NULL);
		break;

	case V4L2_CID_ANALOGUE_GAIN:
		ret = cci_write(gc8034->regmap, GC8034_AGAIN_REG,
				ctrl->val, NULL);
		break;

	case V4L2_CID_VBLANK:
		ret = cci_write(gc8034->regmap, GC8034_FRAME_LENGTH_REG,
				gc8034->cur_mode->height + ctrl->val, NULL);
		break;

	case V4L2_CID_HFLIP:
		ret = gc8034_set_ctrl_hflip(gc8034, ctrl->val);
		break;

	case V4L2_CID_VFLIP:
		ret = gc8034_set_ctrl_vflip(gc8034, ctrl->val);
		break;

	case V4L2_CID_TEST_PATTERN:
		ret = gc8034_test_pattern(gc8034, ctrl->val);
		break;

	default:
		break;
	}

	pm_runtime_put(gc8034->dev);

	return ret;
}

static const struct v4l2_ctrl_ops gc8034_ctrl_ops = {
	.s_ctrl = gc8034_set_ctrl,
};

static int gc8034_identify_module(struct gc8034 *gc8034)
{
	u64 val;
	int ret;

	if (gc8034->identified)
		return 0;

	ret = cci_read(gc8034->regmap, GC8034_REG_CHIP_ID, &val, NULL);
	if (ret)
		return ret;

	if (val != GC8034_CHIP_ID) {
		dev_err(gc8034->dev, "chip id mismatch: 0x%x!=0x%llx",
			GC8034_CHIP_ID, val);
		return -ENXIO;
	}

	gc8034->identified = true;

	return 0;
}

static int gc8034_start_streaming(struct gc8034 *gc8034)
{
	const struct gc8034_mode *mode;
	const struct gc8034_reg_list *reg_list;
	int ret;

	ret = pm_runtime_resume_and_get(gc8034->dev);
	if (ret < 0)
		return ret;

	ret = gc8034_identify_module(gc8034);
	if (ret)
		goto err_rpm_put;

	ret = cci_multi_reg_write(gc8034->regmap,
				  mode_table_common,
				  ARRAY_SIZE(mode_table_common), NULL);
	if (ret)
		goto err_rpm_put;

	mode = gc8034->cur_mode;
	reg_list = &mode->reg_list;

	ret = cci_multi_reg_write(gc8034->regmap,
				  reg_list->regs, reg_list->num_of_regs, NULL);
	if (ret < 0)
		goto err_rpm_put;

	ret = __v4l2_ctrl_handler_setup(&gc8034->ctrls);
	if (ret < 0) {
		dev_err(gc8034->dev, "could not sync v4l2 controls\n");
		goto err_rpm_put;
	}

	ret = cci_write(gc8034->regmap, GC8034_STREAMING_REG, 1, NULL);
	if (ret < 0) {
		dev_err(gc8034->dev, "write STREAMING_REG failed: %d\n", ret);
		goto err_rpm_put;
	}

	return 0;

err_rpm_put:
	pm_runtime_put(gc8034->dev);
	return ret;
}

static int gc8034_stop_streaming(struct gc8034 *gc8034)
{
	int ret;

	ret = cci_write(gc8034->regmap, GC8034_STREAMING_REG, 0, NULL);
	if (ret < 0)
		dev_err(gc8034->dev, "could not sent stop streaming %d\n", ret);

	pm_runtime_put(gc8034->dev);
	return ret;
}

static int gc8034_s_stream(struct v4l2_subdev *subdev, int enable)
{
	struct gc8034 *gc8034 = to_gc8034(subdev);
	struct v4l2_subdev_state *state;
	int ret;

	state = v4l2_subdev_lock_and_get_active_state(subdev);

	if (enable)
		ret = gc8034_start_streaming(gc8034);
	else
		ret = gc8034_stop_streaming(gc8034);

	v4l2_subdev_unlock_state(state);

	return ret;
}

static const struct v4l2_subdev_video_ops gc8034_video_ops = {
	.s_stream = gc8034_s_stream,
};

static const struct v4l2_subdev_pad_ops gc8034_subdev_pad_ops = {
	.enum_mbus_code = gc8034_enum_mbus_code,
	.enum_frame_size = gc8034_enum_frame_size,
	.get_fmt = v4l2_subdev_get_fmt,
	.set_fmt = gc8034_set_format,
	.get_selection = gc8034_get_selection,
};

static const struct v4l2_subdev_ops gc8034_subdev_ops = {
	.video = &gc8034_video_ops,
	.pad = &gc8034_subdev_pad_ops,
};

static const struct v4l2_subdev_internal_ops gc8034_internal_ops = {
	.init_state = gc8034_init_state,
};

static int gc8034_get_regulators(struct device *dev, struct gc8034 *gc8034)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(gc8034_supply_name); i++)
		gc8034->supplies[i].supply = gc8034_supply_name[i];

	return devm_regulator_bulk_get(dev, ARRAY_SIZE(gc8034_supply_name),
				       gc8034->supplies);
}

static int gc8034_parse_fwnode(struct gc8034 *gc8034)
{
	struct fwnode_handle *endpoint;
	struct v4l2_fwnode_endpoint bus_cfg = {
		.bus_type = V4L2_MBUS_CSI2_DPHY,
	};
	int ret;
	struct device *dev = gc8034->dev;

	endpoint =
		fwnode_graph_get_endpoint_by_id(dev_fwnode(dev), 0, 0,
						FWNODE_GRAPH_ENDPOINT_NEXT);
	if (!endpoint)
		return dev_err_probe(dev, -EINVAL, "Missing endpoint node\n");

	ret = v4l2_fwnode_endpoint_alloc_parse(endpoint, &bus_cfg);
	if (ret) {
		dev_err_probe(dev, ret, "parsing endpoint node failed\n");
		goto done;
	}

	ret = v4l2_link_freq_to_bitmap(dev, bus_cfg.link_frequencies,
				       bus_cfg.nr_of_link_frequencies,
				       gc8034_link_freq_menu_items,
				       ARRAY_SIZE(gc8034_link_freq_menu_items),
				       &gc8034->link_freq_bitmap);
	if (ret)
		goto done;

done:
	v4l2_fwnode_endpoint_free(&bus_cfg);
	fwnode_handle_put(endpoint);
	return ret;
}

static u64 gc8034_to_pixel_rate(u32 f_index)
{
	u64 pixel_rate =
		gc8034_link_freq_menu_items[f_index] * 2 * GC8034_DATA_LANES;

	return div_u64(pixel_rate, GC8034_RGB_DEPTH);
}

static int gc8034_init_controls(struct gc8034 *gc8034)
{
	struct i2c_client *client = v4l2_get_subdevdata(&gc8034->sd);
	const struct gc8034_mode *mode = &gc8034_modes[0];
	const struct v4l2_ctrl_ops *ops = &gc8034_ctrl_ops;
	struct v4l2_fwnode_device_properties props;
	struct v4l2_ctrl_handler *ctrl_hdlr;
	s64 exposure_max, h_blank;
	int ret;

	ctrl_hdlr = &gc8034->ctrls;
	ret = v4l2_ctrl_handler_init(ctrl_hdlr, 9);
	if (ret)
		return ret;

	gc8034->hflip = v4l2_ctrl_new_std(ctrl_hdlr, &gc8034_ctrl_ops,
					  V4L2_CID_HFLIP, 0, 1, 1, 0);
	gc8034->vflip = v4l2_ctrl_new_std(ctrl_hdlr, &gc8034_ctrl_ops,
					  V4L2_CID_VFLIP, 0, 1, 1, 0);
	v4l2_ctrl_cluster(2, &gc8034->hflip);

	gc8034->link_freq =
	v4l2_ctrl_new_int_menu(ctrl_hdlr,
			       &gc8034_ctrl_ops,
			       V4L2_CID_LINK_FREQ,
			       ARRAY_SIZE(gc8034_link_freq_menu_items) - 1,
			       0,
			       gc8034_link_freq_menu_items);
	if (gc8034->link_freq)
		gc8034->link_freq->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	gc8034->pixel_rate =
		v4l2_ctrl_new_std(ctrl_hdlr,
				  &gc8034_ctrl_ops,
				  V4L2_CID_PIXEL_RATE, 0,
				  gc8034_to_pixel_rate(0),
				  1,
				  gc8034_to_pixel_rate(0));

	gc8034->vblank =
		v4l2_ctrl_new_std(ctrl_hdlr,
				  &gc8034_ctrl_ops, V4L2_CID_VBLANK,
				  mode->vts_min - mode->height,
				  GC8034_VTS_MAX - mode->height, 1,
				  mode->vts_def - mode->height);

	h_blank = mode->hts - mode->width;
	gc8034->hblank = v4l2_ctrl_new_std(ctrl_hdlr, &gc8034_ctrl_ops,
					   V4L2_CID_HBLANK, h_blank, h_blank, 1,
					   h_blank);
	if (gc8034->hblank)
		gc8034->hblank->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	v4l2_ctrl_new_std(ctrl_hdlr, &gc8034_ctrl_ops,
			  V4L2_CID_ANALOGUE_GAIN, GC8034_AGAIN_MIN,
			  GC8034_AGAIN_MAX, GC8034_AGAIN_STEP,
			  GC8034_AGAIN_MIN);

	exposure_max = mode->vts_def - GC8034_EXP_MARGIN;
	gc8034->exposure = v4l2_ctrl_new_std(ctrl_hdlr, &gc8034_ctrl_ops,
					     V4L2_CID_EXPOSURE, GC8034_EXP_MIN,
					     exposure_max, GC8034_EXP_STEP,
					     exposure_max);

	v4l2_ctrl_new_std_menu_items(ctrl_hdlr, &gc8034_ctrl_ops,
				     V4L2_CID_TEST_PATTERN,
				     ARRAY_SIZE(gc8034_test_pattern_menu) - 1,
				     0, 0, gc8034_test_pattern_menu);

	/* register properties to fwnode (e.g. rotation, orientation) */
	ret = v4l2_fwnode_device_parse(&client->dev, &props);
	if (ret)
		goto error_ctrls;

	ret = v4l2_ctrl_new_fwnode_properties(ctrl_hdlr, ops, &props);
	if (ret)
		goto error_ctrls;

	if (ctrl_hdlr->error) {
		ret = ctrl_hdlr->error;
		goto error_ctrls;
	}

	gc8034->sd.ctrl_handler = ctrl_hdlr;

	return 0;

error_ctrls:
	v4l2_ctrl_handler_free(ctrl_hdlr);

	return ret;
}

static int gc8034_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct gc8034 *gc8034;
	int ret;

	gc8034 = devm_kzalloc(dev, sizeof(*gc8034), GFP_KERNEL);
	if (!gc8034)
		return -ENOMEM;

	gc8034->dev = dev;

	ret = gc8034_parse_fwnode(gc8034);
	if (ret)
		return ret;

	gc8034->regmap = devm_cci_regmap_init_i2c(client, 16);
	if (IS_ERR(gc8034->regmap))
		return dev_err_probe(dev, PTR_ERR(gc8034->regmap),
				     "failed to init CCI\n");

	gc8034->xclk = devm_v4l2_sensor_clk_get_legacy(dev, NULL, true,
						       GC8034_DEFAULT_CLK_FREQ);
	if (IS_ERR(gc8034->xclk))
		return dev_err_probe(dev, PTR_ERR(gc8034->xclk),
				     "failed to get xclk\n");

	ret = gc8034_get_regulators(dev, gc8034);
	if (ret < 0)
		return dev_err_probe(dev, ret,
				     "failed to get regulators\n");

	gc8034->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(gc8034->reset_gpio))
		return dev_err_probe(dev, PTR_ERR(gc8034->reset_gpio),
				     "failed to get gpio\n");

	v4l2_i2c_subdev_init(&gc8034->sd, client, &gc8034_subdev_ops);
	gc8034->sd.internal_ops = &gc8034_internal_ops;
	gc8034->cur_mode = &gc8034_modes[0];

	ret = gc8034_init_controls(gc8034);
	if (ret)
		return dev_err_probe(dev, ret,
				     "failed to init controls\n");

	gc8034->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	gc8034->pad.flags = MEDIA_PAD_FL_SOURCE;
	gc8034->sd.dev = &client->dev;
	gc8034->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;

	ret = media_entity_pads_init(&gc8034->sd.entity, 1, &gc8034->pad);
	if (ret < 0) {
		dev_err_probe(dev, ret, "could not register media entity\n");
		goto err_v4l2_ctrl_handler_free;
	}

	gc8034->sd.state_lock = gc8034->ctrls.lock;
	ret = v4l2_subdev_init_finalize(&gc8034->sd);
	if (ret < 0) {
		dev_err_probe(dev, ret, "v4l2 subdev init error\n");
		goto err_media_entity_cleanup;
	}

	pm_runtime_enable(gc8034->dev);
	pm_runtime_set_autosuspend_delay(gc8034->dev, 1000);
	pm_runtime_use_autosuspend(gc8034->dev);
	pm_runtime_idle(gc8034->dev);

	ret = v4l2_async_register_subdev_sensor(&gc8034->sd);
	if (ret < 0) {
		dev_err_probe(dev, ret, "could not register v4l2 device\n");
		goto err_rpm;
	}

	return 0;

err_rpm:
	pm_runtime_disable(gc8034->dev);
	v4l2_subdev_cleanup(&gc8034->sd);

err_media_entity_cleanup:
	media_entity_cleanup(&gc8034->sd.entity);

err_v4l2_ctrl_handler_free:
	v4l2_ctrl_handler_free(&gc8034->ctrls);

	return ret;
}

static void gc8034_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct gc8034 *gc8034 = to_gc8034(sd);

	v4l2_async_unregister_subdev(&gc8034->sd);
	v4l2_subdev_cleanup(sd);
	media_entity_cleanup(&gc8034->sd.entity);
	v4l2_ctrl_handler_free(&gc8034->ctrls);

	pm_runtime_disable(&client->dev);
	if (!pm_runtime_status_suspended(&client->dev))
		gc8034_power_off(gc8034->dev);
	pm_runtime_set_suspended(&client->dev);
}

static const struct of_device_id gc8034_of_match[] = {
	{ .compatible = "galaxycore,gc8034" },
	{}
};
MODULE_DEVICE_TABLE(of, gc8034_of_match);

static DEFINE_RUNTIME_DEV_PM_OPS(gc8034_pm_ops,
				 gc8034_power_off,
				 gc8034_power_on,
				 NULL);

static struct i2c_driver gc8034_i2c_driver = {
	.driver = {
		.of_match_table = gc8034_of_match,
		.pm = pm_ptr(&gc8034_pm_ops),
		.name  = "gc8034",
	},
	.probe = gc8034_probe,
	.remove = gc8034_remove,
};
module_i2c_driver(gc8034_i2c_driver);

MODULE_DESCRIPTION("GalaxyCore gc8034 Camera driver");
MODULE_AUTHOR("Zhi Mao <zhi.mao@mediatek.com>");
MODULE_LICENSE("GPL");
