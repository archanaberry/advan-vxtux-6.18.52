/*
 * Minimal type surface for the Spreadtrum camera flash HAL.
 *
 * Port 2026-10-04. flash_drv.c and flash_drv.h are the vendor's own sources
 * (work/donorcatalog/mods-sprd/sprd/common/camera/flash/flash_drv/, Copyright
 * Spreadtrum) -- the driver did NOT have to be reverse engineered after all.
 * What was missing is only this type surface: the vendor builds flash_drv.c
 * against the full 25 KB sprd_img.h, which drags in the entire ISP image
 * pipeline. This header carries just the five flash structs that HAL actually
 * touches, copied verbatim from that sprd_img.h so the layouts are identical.
 *
 * Do not edit the struct bodies: flash chip drivers in this tree are compiled
 * against them.
 */
#ifndef _SPRD_FLASH_TYPES_H_
#define _SPRD_FLASH_TYPES_H_

#include <linux/types.h>


/* Copied verbatim from sprd_img.h (interface/), lines 170-206. flash_drv.c
 * switches on these, so the values are part of the ABI with the ISP core. */
enum sprd_flash_type {
	FLASH_TYPE_PREFLASH,
	FLASH_TYPE_MAIN,
	FLASH_TYPE_TORCH,
	FLASH_TYPE_MAX
};

enum sprd_flash_io_id {
	FLASH_IOID_GET_CHARGE,
	FLASH_IOID_GET_TIME,
	FLASH_IOID_GET_MAX_CAPACITY,
	FLASH_IOID_SET_CHARGE,
	FLASH_IOID_SET_TIME,
	FLASH_IOID_MAX
};

enum sprd_flash_status {
	FLASH_CLOSE = 0x0,
	FLASH_OPEN = 0x1,
	FLASH_TORCH = 0x2,	/* user only set flash to close/open/torch state */
	FLASH_AUTO = 0x3,
	FLASH_CLOSE_AFTER_OPEN = 0x10,	/* following is set to sensor */
	FLASH_HIGH_LIGHT = 0x11,
	FLASH_OPEN_ON_RECORDING = 0x22,
	FLASH_CLOSE_AFTER_AUTOFOCUS = 0x30,
	FLASH_NEED_QUIT = 0x31,
	FLASH_AF_DONE = 0x40,
	FLASH_WAIT_TO_CLOSE = 0x41,
	FLASH_CURRENT_LEVEL_SET = 0x42,
	FLASH_STATUS_MAX
};

#define SPRD_FLASH_MAX_CELL	40

struct sprd_flash_element {
	uint16_t index;
	uint16_t val;
	uint16_t brightness;
	uint16_t color_temp;
	uint32_t bg_color;
};

struct sprd_flash_capacity {
	uint16_t max_charge;
	uint16_t max_time;
	uint16_t max_torch_current;
	uint16_t max_preflash_current;
	uint16_t max_highlight_current;
	float torch_step_current;
	float preflash_step_current;
	float highlight_step_current;
	uint16_t max_torch_current_total;
	uint16_t max_preflash_current_total;
	uint16_t max_highlight_current_total;
	uint16_t torch_steps;
	uint16_t preflash_steps;
	uint16_t highlight_steps;
	char *flash_ic_name;
};

struct sprd_img_set_flash {
	uint32_t led0_ctrl;
	uint32_t led1_ctrl;
	uint32_t led0_status;
	uint32_t led1_status;
	uint32_t flash_index;
};

struct sprd_flash_cell {
	uint8_t type;
	uint8_t count;
	uint8_t def_val;
	uint8_t led_idx;
	struct sprd_flash_element element[SPRD_FLASH_MAX_CELL];
};

struct sprd_flash_cfg_param {
	uint32_t io_id;
	uint8_t flash_idx;
	struct sprd_flash_cell real_cell;
};

#endif /* _SPRD_FLASH_TYPES_H_ */
