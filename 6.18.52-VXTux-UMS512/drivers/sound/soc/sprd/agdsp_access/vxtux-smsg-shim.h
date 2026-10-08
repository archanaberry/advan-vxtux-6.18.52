/* SPDX-License-Identifier: GPL-2.0
 *
 * vxtux-smsg-shim.h — shim smsg vendor (linux/sipc.h) -> layer aud_smsg port.
 * Hanya untuk agdsp_access.c (thread agdsp_access_monitor).
 *
 * RE ground truth (RE via llvm-nm blob vendor + sumber GPL fork):
 *   struct smsg { u8 channel; u8 type; u16 flag; u32 value; }
 *   smsg_ch_open(dst,ch,tmo), smsg_ch_close(dst,ch,tmo),
 *   smsg_set(msg,ch,type,flag,value), smsg_open_ack(dst,ch),
 *   smsg_recv(dst,msg,tmo), smsg_send(dst,msg,tmo)
 *   SMSG_TYPE_OPEN/CMD/DONE
 *
 * Padanan port (drivers/vxtux/audio/audiosipc, diekspor):
 *   aud_smsg_ch_open(dst,ch)      — tanpa timeout (menunggu internal)
 *   aud_smsg_ch_close(dst,ch)
 *   aud_smsg_set(msg,ch,cmd,v0..v3) — command ~ type, parameter0 ~ flag,
 *                                     parameter1 ~ value
 *   aud_smsg_recv(dst,msg,tmo)    — sama
 *   aud_smsg_send(dst,msg)        — tanpa timeout
 *
 * Tipe pesan: smsg vendor OPEN=1..DONE=6 (fork sipc.h); aud layer tidak
 * menomboki tipe (command bebas) — DSP tetap mengenal nilai 1..6 protokol
 * lama, jadi shim memakai angka yang sama (bukan makna baru).
 */
#ifndef _VXTUX_AGDSP_SMSG_SHIM_H
#define _VXTUX_AGDSP_SMSG_SHIM_H

#include "../audiosipc/audio-smsg.h"

/* nilai protokol dari fork sipc.h (SMSG_TYPE enum) — dipertahankan agar
 * pesan di kabel identik dengan yang dibekukan di firmware AGDSP */
enum {
	VXTUX_SMSG_TYPE_OPEN = 1,
	VXTUX_SMSG_TYPE_CMD = 5,
	VXTUX_SMSG_TYPE_DONE = 6,
};

struct smsg {
	u8	channel;
	u8	type;
	u16	flag;
	u32	value;
};

static inline int smsg_ch_open(u8 dst, u8 channel, int timeout)
{
	(void)timeout;	/* aud layer menangani tunggunya sendiri */
	return aud_smsg_ch_open(dst, channel);
}

static inline int smsg_ch_close(u8 dst, u8 channel, int timeout)
{
	(void)timeout;
	return aud_smsg_ch_close(dst, channel);
}

static inline void smsg_set(struct smsg *msg, u8 channel,
			    u8 type, u16 flag, u32 value)
{
	msg->channel = channel;
	msg->type = type;
	msg->flag = flag;
	msg->value = value;
}

static inline int smsg_recv(u8 dst, struct smsg *msg, int timeout)
{
	struct aud_smsg am;
	int rval;

	memset(&am, 0, sizeof(am));
	am.channel = msg->channel;
	rval = aud_smsg_recv(dst, &am, timeout);
	if (rval)
		return rval;

	/* aud_smsg: command ~ type, parameter0 ~ flag, parameter1 ~ value */
	msg->type = (u8)am.command;
	msg->flag = (u16)am.parameter0;
	msg->value = am.parameter1;

	return 0;
}

static inline int smsg_send(u8 dst, struct smsg *msg, int timeout)
{
	struct aud_smsg am;

	(void)timeout;
	memset(&am, 0, sizeof(am));
	am.channel = msg->channel;
	am.command = msg->type;
	am.parameter0 = msg->flag;
	am.parameter1 = msg->value;

	return aud_smsg_send(dst, &am);
}

/* OPEN handshake ditangani state-mesin channel di aud layer; ack lokal
 * tidak diperlukan (aud_smsg_ch_open sudah menandai CHAN_STATE_OPENED). */
static inline void smsg_open_ack(u8 dst, u16 channel)
{
	(void)dst;
	(void)channel;
}

#define SMSG_TYPE_OPEN		VXTUX_SMSG_TYPE_OPEN
#define SMSG_TYPE_CMD		VXTUX_SMSG_TYPE_CMD
#define SMSG_TYPE_DONE		VXTUX_SMSG_TYPE_DONE

#endif /* _VXTUX_AGDSP_SMSG_SHIM_H */
