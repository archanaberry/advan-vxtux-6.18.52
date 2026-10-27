/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _XT_TCPMSS_MATCH_H
#define _XT_TCPMSS_MATCH_H

#include <linux/types.h>

struct xt_tcpmss_match_info {
    __u16 mss_min, mss_max;
    __u8 invert;
};

/* VXTux 6.18 backport: TCPMSS target uses these constants. The target
 * struct was kept in upstream 6.18's xt_tcpmss.c so this header has to
 * expose the target layout alongside the match. The values match the
 * pre-6.18 uapi definitions. */
#define XT_TCPMSS_CLAMP_PMTU	0xffffU

struct xt_tcpmss_info {
	__u16 mss;
};

#endif /*_XT_TCPMSS_MATCH_H*/
