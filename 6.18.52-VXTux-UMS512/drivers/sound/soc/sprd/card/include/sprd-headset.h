/*
 * Copyright (C) 2015 Spreadtrum Communications Inc.
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

#ifndef __HEADSET_SPRD_H__
#define __HEADSET_SPRD_H__

/*
 * Port VXTux (6.18): gerbang asli memakai symbol vendor
 * CONFIG_SND_SOC_SPRD_CODEC_*, sedangkan port ini memakai
 * CONFIG_VXTUX_SND_SPRD_SC2730. Tanpa cabang ini seluruh struct headset
 * (sprd_headset, sprd_headset_platform_data, headset_power, headset_buttons)
 * tetap tak terdefinisi -> 611 error di codec_sc2730/sprd-headset-sc2730.c.
 * Path relatif "sprd-headset-2730.h" diselesaikan lewat -I codec_sc2730/
 * (lihat card/Makefile & codec_sc2730/Makefile).
 */
#if defined(CONFIG_VXTUX_SND_SPRD_SC2730)
#include "sprd-headset-2730.h"
#elif defined(CONFIG_SND_SOC_SPRD_CODEC_SC2730)
#include "./sc2730/sprd-headset-2730.h"
#elif defined(CONFIG_SND_SOC_SPRD_CODEC_UMP9620)
#include "./ump9620/sprd-headset-ump9620.h"
#elif defined(CONFIG_SND_SOC_SPRD_CODEC_SC2721)
#include "./sc2721/sprd-headset-2721.h"
#endif

#endif
