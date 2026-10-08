/* SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR MIT) */
/*
 * Rockchip preisp driver
 * Copyright (C) 2018 Rockchip Electronics Co., Ltd.
 *
 * Port 2026-10-04 (lindroid4 T618/UMS512).
 *
 * WHY THIS FILE EXISTS
 * --------------------
 * drivers/media/platform/sprd/sensors/s5k4h7yx.c does
 *     #include <linux/rk-preisp.h>
 * inherited from a Rockchip sensor tree. That header was in neither this kernel
 * nor any donor, so this sensor had never been compiled.
 *
 * Content is Rockchip's own UAPI header, taken verbatim from their kernel-5.10
 * tree. The struct layouts and the PREISP_* ioctl numbers are a user ABI shared
 * with Rockchip's preisp userspace, so they are reproduced exactly rather than
 * trimmed: _IOW/_IOR encode sizeof(arg), and changing any member would silently
 * corrupt the ioctl.
 *
 * Nothing in this tree currently issues a PREISP_* ioctl -- s5k4h7yx.c only
 * uses the macros it needs to talk to a preisp node that this device does not
 * have. The header is present so the file can compile and bind correctly if a
 * preisp node is ever added; see recovery/camera_sensors_blocked/MANIFEST.json.
 *
 * BASE_VIDIOC_PRIVATE comes from <linux/videodev2.h>, which every includer pulls
 * in before this header.
 */

#ifndef _UAPI_RKPREISP_H
#define _UAPI_RKPREISP_H

#include <linux/types.h>

#define PREISP_FW_NAME_LEN		128
#define PREISP_LSCTBL_SIZE		289

#define PREISP_CMD_SET_HDRAE_EXP \
	_IOW('V', BASE_VIDIOC_PRIVATE + 0, struct preisp_hdrae_exp_s)

#define PREISP_CMD_SAVE_HDRAE_PARAM \
	_IOW('V', BASE_VIDIOC_PRIVATE + 1, struct preisp_hdrae_para_s)

#define PREISP_DISP_SET_FRAME_OUTPUT \
	_IOW('V', BASE_VIDIOC_PRIVATE + 4, int)

#define PREISP_DISP_SET_FRAME_FORMAT \
	_IOW('V', BASE_VIDIOC_PRIVATE + 5, unsigned int)
#define PREISP_DISP_SET_FRAME_TYPE \
	_IOW('V', BASE_VIDIOC_PRIVATE + 6, unsigned int)

#define PREISP_DISP_SET_PRO_TIME \
	_IOW('V', BASE_VIDIOC_PRIVATE + 7, unsigned int)

#define PREISP_DISP_SET_PRO_CURRENT \
	_IOW('V', BASE_VIDIOC_PRIVATE + 8, unsigned int)

#define PREISP_DISP_SET_DENOISE \
	_IOW('V', BASE_VIDIOC_PRIVATE + 9, unsigned int[2])

#define PREISP_DISP_WRITE_EEPROM \
	_IO('V', BASE_VIDIOC_PRIVATE + 10)

#define PREISP_DISP_READ_EEPROM \
	_IO('V', BASE_VIDIOC_PRIVATE + 11)

#define PREISP_DISP_SET_LED_ON_OFF \
	_IOW('V', BASE_VIDIOC_PRIVATE + 12, unsigned int)

#define PREISP_POWER_ON		_IO('p',   1)
#define PREISP_POWER_OFF	_IO('p',   2)
#define PREISP_REQUEST_SLEEP	_IOW('p',  3, __s32)
#define PREISP_WAKEUP		_IO('p',   4)
#define PREISP_DOWNLOAD_FW	_IOW('p',  5, char[PREISP_FW_NAME_LEN])
#define PREISP_WRITE		_IOW('p',  6, struct preisp_apb_pkt)
#define PREISP_READ		_IOR('p',  7, struct preisp_apb_pkt)
#define PREISP_ST_QUERY		_IOR('p',  8, __s32)
#define PREISP_IRQ_REQUEST	_IOW('p',  9, __s32)
#define PREISP_SEND_MSG		_IOW('p', 11, __s32)
#define PREISP_QUERY_MSG		_IOR('p', 12, __s32)
#define PREISP_RECV_MSG		_IOR('p', 13, __s32)
#define PREISP_CLIENT_CONNECT	_IOW('p', 15, __s32)
#define PREISP_CLIENT_DISCONNECT _IO('p', 16)

struct preisp_apb_pkt {
	__s32 data_len;
	__s32 addr;
	__s32 *data;
};

struct preisp_hdrae_para_s {
	unsigned short r_gain;
	unsigned short b_gain;
	unsigned short gr_gain;
	unsigned short gb_gain;
	int lsc_table[PREISP_LSCTBL_SIZE];
};

enum cg_mode_e {
	GAIN_MODE_LCG,
	GAIN_MODE_HCG,
};

struct preisp_hdrae_exp_s {
	unsigned int long_exp_reg;
	unsigned int long_gain_reg;
	unsigned int middle_exp_reg;
	unsigned int middle_gain_reg;
	unsigned int short_exp_reg;
	unsigned int short_gain_reg;
	unsigned int long_exp_val;
	unsigned int long_gain_val;
	unsigned int middle_exp_val;
	unsigned int middle_gain_val;
	unsigned int short_exp_val;
	unsigned int short_gain_val;
	unsigned char long_cg_mode;
	unsigned char middle_cg_mode;
	unsigned char short_cg_mode;
};

#endif /* _UAPI_RKPREISP_H */
