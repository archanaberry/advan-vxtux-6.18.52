#ifndef __WCN_DUMP_INTEGRATE_H__
#define __WCN_DUMP_INTEGRATE_H__

#define WCN_DUMP_END_STRING "marlin_memdump_finish"
#define WCN_CP2_STATUS_DUMP_REG	0x6a6b6c6d

#define DUMP_PACKET_SIZE	(1024)

typedef void (*gnss_dump_callback) (void);
void mdbg_dump_gnss_register(
			gnss_dump_callback callback_func, void *para);
void mdbg_dump_gnss_unregister(void);
int mdbg_snap_shoot_iram(void *buf);
/*
 * mdbg_dump_mem_integ(), BUKAN mdbg_dump_mem(): versi SDIO di wcn_dump.c
 * menyediakan `int mdbg_dump_mem(void)` (lihat wcn_dump.h), sementara berkas
 * ini mengimplementasikan `void mdbg_dump_mem_integ(void)`. Deklarasi lama
 * dengan nama yang sama membuat error "conflicting types" di setiap berkas
 * yang menyertakan kedua header (mis. gnss_dump.c, wcn_procfs.c).
 */
void mdbg_dump_mem_integ(void);
int dump_arm_reg(void);
u32 mdbg_check_wifi_ip_status(void);
u32 mdbg_check_bt_poweron(void);
u32 mdbg_check_gnss_poweron(void);
u32 mdbg_check_wcn_sys_exit_sleep(void);
u32 mdbg_check_btwf_sys_exit_sleep(void);

#endif
