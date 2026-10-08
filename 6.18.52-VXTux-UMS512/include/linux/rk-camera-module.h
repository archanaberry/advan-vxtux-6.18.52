/* SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR MIT) */
/*
 * Rockchip camera module information -- minimal subset.
 * Copyright (C) 2018-2019 Rockchip Electronics Co., Ltd.
 *
 * WHY THIS FILE EXISTS (port 2026-10-04, lindroid4 T618/UMS512)
 * --------------------------------------------------------------
 * drivers/media/platform/sprd/sensors/gc5025.c and s5k4h7yx.c both do
 *     #include <linux/rk-camera-module.h>
 * inherited from a Rockchip vendor kernel. That header is not present in this
 * tree and is not in any donor, so those two sensors had never been compiled.
 *
 * This is the authentic upstream Rockchip ABI, trimmed to the 13 symbols the
 * two files actually reference. Values are taken verbatim from Rockchip's
 * rk-camera-module.h -- notably RKMODULE_NAME_LEN 32, the DT property strings
 * "rockchip,camera-module-*", and the ioctl command numbers, which are
 * BASE_VIDIOC_PRIVATE+0/1/4/5/10. The struct layouts are a kernel/userspace
 * ABI shared with rkaiq, so they are reproduced whole rather than reduced:
 * rkmodule_inf embeds base/fac/awb/lsc/af/pdaf/module_inf by value, and
 * shrinking any member would silently corrupt the ioctl.
 *
 * Trimmed away: the lvds, dpcc, nr-switch, bt656, vicap-reset, vicap-fmt and
 * sync-mode definitions. Nothing in this tree references them; if a future
 * sensor does, restore that part from upstream rather than guessing.
 *
 * BASE_VIDIOC_PRIVATE comes from <linux/videodev2.h>, which every includer
 * already pulls in before this header.
 */

#ifndef _UAPI_RKMODULE_CAMERA_H
#define _UAPI_RKMODULE_CAMERA_H

#include <linux/types.h>

#define RKMODULE_API_VERSION KERNEL_VERSION(0, 1, 0x2)

#define RKMODULE_NAME_LEN 32
#define RKMODULE_LSCDATA_LEN 289

#define RKMODULE_MAX_VC_CH 4

#define RKMODULE_PADF_GAINMAP_LEN 1024
#define RKMODULE_PDAF_DCCMAP_LEN 256
#define RKMODULE_AF_OTP_MAX_LEN 3

#define RKMODULE_CAMERA_MODULE_INDEX "rockchip,camera-module-index"
#define RKMODULE_CAMERA_MODULE_FACING "rockchip,camera-module-facing"
#define RKMODULE_CAMERA_MODULE_NAME "rockchip,camera-module-name"
#define RKMODULE_CAMERA_LENS_NAME "rockchip,camera-module-lens-name"

/* ioctl commands. Values are Rockchip's rkaiq ABI and must not be
 * renumbered; BASE_VIDIOC_PRIVATE is 0x56. Only the commands whose
 * argument struct is defined below are present -- _IOR/_IOW encode
 * sizeof(arg), so a command whose struct is absent cannot be
 * declared at all. RKMODULE_AF_CFG, _LSC_CFG, _SET_HDR_CFG and
 * _SET_QUICK_STREAM are kept; the lvds/dpcc/nr set is omitted along
 * with its structs. */
#define RKMODULE_GET_MODULE_INFO _IOR('V', BASE_VIDIOC_PRIVATE + 0, struct rkmodule_inf)
#define RKMODULE_AWB_CFG         _IOW('V', BASE_VIDIOC_PRIVATE + 1, struct rkmodule_awb_cfg)
#define RKMODULE_AF_CFG          _IOW('V', BASE_VIDIOC_PRIVATE + 2, struct rkmodule_af_cfg)
#define RKMODULE_LSC_CFG         _IOW('V', BASE_VIDIOC_PRIVATE + 3, struct rkmodule_lsc_cfg)
#define RKMODULE_GET_HDR_CFG     _IOR('V', BASE_VIDIOC_PRIVATE + 4, struct rkmodule_hdr_cfg)
#define RKMODULE_SET_HDR_CFG     _IOW('V', BASE_VIDIOC_PRIVATE + 5, struct rkmodule_hdr_cfg)
#define RKMODULE_SET_CONVERSION_GAIN _IOW('V', BASE_VIDIOC_PRIVATE + 6, __u32)
#define RKMODULE_SET_QUICK_STREAM _IOW('V', BASE_VIDIOC_PRIVATE + 10, __u32)

struct rkmodule_base_inf {
	char sensor[RKMODULE_NAME_LEN];
	char module[RKMODULE_NAME_LEN];
	char lens[RKMODULE_NAME_LEN];
} __attribute__((packed));

struct rkmodule_fac_inf {
	__u32 flag;
	char module[RKMODULE_NAME_LEN];
	char lens[RKMODULE_NAME_LEN];
	__u32 year;
	__u32 month;
	__u32 day;
} __attribute__((packed));

struct rkmodule_awb_inf {
	__u32 flag;
	__u32 r_value;
	__u32 b_value;
	__u32 gr_value;
	__u32 gb_value;
	__u32 golden_r_value;
	__u32 golden_b_value;
	__u32 golden_gr_value;
	__u32 golden_gb_value;
} __attribute__((packed));

struct rkmodule_lsc_inf {
	__u32 flag;
	__u16 lsc_w;
	__u16 lsc_h;
	__u16 decimal_bits;
	__u16 lsc_r[RKMODULE_LSCDATA_LEN];
	__u16 lsc_b[RKMODULE_LSCDATA_LEN];
	__u16 lsc_gr[RKMODULE_LSCDATA_LEN];
	__u16 lsc_gb[RKMODULE_LSCDATA_LEN];
	__u16 width;
	__u16 height;
	__u16 table_size;
} __attribute__((packed));

enum rkmodele_af_otp_dir {
	AF_OTP_DIR_HORIZONTAL = 0,
	AF_OTP_DIR_UP = 1,
	AF_OTP_DIR_DOWN = 2,
};

struct rkmodule_af_otp {
	__u32 vcm_start;
	__u32 vcm_end;
	__u32 vcm_dir;
};

struct rkmodule_af_inf {
	__u32 flag;
	__u32 dir_cnt;
	struct rkmodule_af_otp af_otp[RKMODULE_AF_OTP_MAX_LEN];
} __attribute__((packed));

struct rkmodule_pdaf_inf {
	__u32 flag;
	__u32 gainmap_width;
	__u32 gainmap_height;
	__u32 dccmap_width;
	__u32 dccmap_height;
	__u32 dcc_mode;
	__u32 dcc_dir;
	__u16 gainmap[RKMODULE_PADF_GAINMAP_LEN];
	__u16 dccmap[RKMODULE_PDAF_DCCMAP_LEN];
} __attribute__((packed));

struct rkmodule_otp_module_inf {
	__u32 flag;
	__u8 vendor[8];
	__u32 module_id;
	__u16 version;
	__u16 full_width;
	__u16 full_height;
	__u8 supplier_id;
	__u8 year;
	__u8 mouth;
	__u8 day;
	__u8 sensor_id;
	__u8 lens_id;
	__u8 vcm_id;
	__u8 drv_id;
	__u8 flip;
} __attribute__((packed));

struct rkmodule_inf {
	struct rkmodule_base_inf base;
	struct rkmodule_fac_inf fac;
	struct rkmodule_awb_inf awb;
	struct rkmodule_lsc_inf lsc;
	struct rkmodule_af_inf af;
	struct rkmodule_pdaf_inf pdaf;
	struct rkmodule_otp_module_inf module_inf;
} __attribute__((packed));

struct rkmodule_awb_cfg {
	__u32 enable;
	__u32 golden_r_value;
	__u32 golden_b_value;
	__u32 golden_gr_value;
	__u32 golden_gb_value;
} __attribute__((packed));

struct rkmodule_af_cfg {
	__u32 enable;
	__u32 vcm_start;
	__u32 vcm_end;
	__u32 vcm_dir;
} __attribute__((packed));

struct rkmodule_lsc_cfg {
	__u32 enable;
} __attribute__((packed));

enum rkmodule_hdr_mode {
	NO_HDR = 0,
	HDR_X2 = 5,
	HDR_X3 = 6,
};

enum hdr_esp_mode {
	HDR_NORMAL_VC = 0,
	HDR_LINE_CNT,
	HDR_ID_CODE,
};

struct rkmodule_hdr_esp {
	enum hdr_esp_mode mode;
	union {
		struct {
			__u32 padnum;
			__u32 padpix;
		} lcnt;
		struct {
			__u32 efpix;
			__u32 obpix;
		} idcd;
	} val;
};

struct rkmodule_hdr_cfg {
	__u32 hdr_mode;
	struct rkmodule_hdr_esp esp;
} __attribute__((packed));


#endif /* _UAPI_RKMODULE_CAMERA_H */