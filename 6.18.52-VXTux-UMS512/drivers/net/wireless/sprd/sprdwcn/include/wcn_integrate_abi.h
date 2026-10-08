/* SPDX-License-Identifier: GPL-2.0 */
/*
 * wcn_integrate_abi.h — konstanta & tipe BERSAMA lapisan integrate WCN.
 *
 * Kenapa berkas ini ada (2026-09-23, akar build #7):
 *   `boot/wcn_integrate.h` memakai GNSS_EFUSE_BLOCK_COUNT (baris 132),
 *   struct wifi_calibration (136), WIFI_EFUSE_BLOCK_COUNT (137, 227), tetapi
 *   ketiganya didefinisikan di `boot/wcn_integrate_dev.h` SETELAH baris
 *   `#include "wcn_glb.h"` (baris 5). Jalur include
 *       dev.h -> wcn_glb.h -> wcn_integrate_glb.h -> wcn_integrate.h
 *   karena itu tiba di wcn_integrate.h sebelum definisinya ada -> 8 error
 *   "use of undeclared identifier" / "field has incomplete type".
 *   Memindahkan definisi ke daun (berkas ini) memutus siklus tanpa menyalin:
 *   satu pemilik, dua pemakai.
 *
 *   Berkas yang sama juga menutup kelas error kedua: `struct wcn_device` hanya
 *   punya SATU definisi (boot/wcn_integrate_dev.h:236), tetapi prototipe di
 *   wcn_integrate.h:240 memakainya lebih dulu. Tag yang pertama disebut di dalam
 *   daftar parameter prototipe punya *prototype scope* — tipe yang BERBEDA dari
 *   struct file-scope yang muncul kemudian — sehingga clang melaporkan
 *   "incompatible pointer types passing 'struct wcn_device *' to parameter of
 *   type 'struct wcn_device *'" (wcn_integrate_boot.c:99/100/757). Deklarasi
 *   maju file-scope di bawah ini membuat keduanya tipe yang sama.
 *
 * Semua nilai di bawah disalin dari header vendor di pohon ini; tidak ada yang
 * dikarang:
 *   include/sc2342_glb.h:5-8            MARLIN_CP_INIT_* (SC2342)
 *   include/umw2631_integrate_glb.h:5-8 MARLIN_CP_INIT_* (UMW2631)
 *   boot/wcn_integrate_dev.h:199-216    struct wifi_calibration + jumlah blok EFUSE
 */
#ifndef __WCN_INTEGRATE_ABI_H__
#define __WCN_INTEGRATE_ABI_H__

/* Jalur relatif terhadap BERKAS INI (bukan -I), supaya header ini aman dari
 * direktori mana pun ia di-include (boot/ punya -I../platform/rf, platform/
 * punya -Irf, tapi TU lain belum tentu). */
#include "../platform/rf/rf.h"	/* struct wifi_config_t, struct wifi_cali_t */
#include <misc/marlin_platform.h>	/* enum wcn_clock_type, enum wcn_clock_mode */

/* Deklarasi maju file-scope: WAJIB sebelum prototipe mana pun yang menyebut
 * struct wcn_device (lihat catatan di atas). Definisi tunggalnya di
 * boot/wcn_integrate_dev.h. */
struct wcn_device;

struct wifi_calibration {
	struct wifi_config_t config_data;
	struct wifi_cali_t cali_data;
};

/* Satu pemilik untuk tag ini (2026-09-23): dulu didefinisikan di boot/wcn_integrate
 * _dev.h:222 DAN platform/wcn_boot.h:60 dengan bentuk IDENTIK -> "redefinition"
 * begitu kedua dunia muncul di satu TU (error build #8). Bentuknya disalin apa
 * adanya dari kedua salinan itu; komentar gpio diambil dari salinan platform. */
struct wcn_clock_info {
	enum wcn_clock_type type;
	enum wcn_clock_mode mode;
	/*
	 * xtal-26m-clk-type-gpio config in the dts.
	 * if xtal-26m-clk-type config in the dts,this gpio unvalid.
	 */
	int gpio;
};

/* Data efuse; nilai default dari tim PHY (sumber: boot/wcn_integrate_dev.h). */
#define WIFI_EFUSE_BLOCK_COUNT	(3)
#define WCN_EFUSE_BLOCK_COUNT	(4)
#define GNSS_EFUSE_BLOCK_COUNT	(3)

/*
 * Protokol boot CP marlin. Nama ini tidak didefinisikan untuk chip SDIO
 * (sc2355_glb.h tidak memilikinya) karena jalur boot marlin memang hanya ada di
 * chip integrated.
 *
 * READY berbeda antar chip vendor:
 *   sc2342_glb.h:5           -> 0xababbaba
 *   umw2631_integrate_glb.h:5 -> 0xf0f0f0ff
 * START/SUCCESS/FAILED identik di kedua header.
 *
 * Blob perangkat memilih READY saat RUNTIME, bukan compile-time — dibaca
 * langsung dari wcn_bsp.ko (blobs/vendor_dlkm) alamat 0x2dcf8..0x2dd10:
 *
 *     2dcf8: mov   w9,  #0xbaba
 *     2dcfc: mov   w10, #0xf0ff
 *     2dd00: cmp   w8,  #0x3            ; w8 = wcn_platform_chip_type()
 *     2dd08: movk  w9,  #0xabab, lsl #16 ; 0xababbaba
 *     2dd0c: movk  w10, #0xf0f0, lsl #16 ; 0xf0f0f0ff
 *     2dd10: csel  w23, w10, w9, eq      ; type==3 ? umw2631 : sc2342
 *     2dd40: cmp   w8, w23               ; bandingkan nilai dari phy
 *
 * Nilai 3 = WCN_PLATFORM_TYPE_QOGIRL6 (boot/wcn_integrate.h:99, enum ke-4).
 * Karena itu wcn_integrate_boot.c memakai pemilih runtime (lihat
 * marlin_cp_init_ready_magic()), bukan salah satu makro di bawah.
 */
#define MARLIN_CP_INIT_START_MAGIC		(0x5a5a5a5a)
#define MARLIN_CP_INIT_SUCCESS_MAGIC		(0x13579bdf)
#define MARLIN_CP_INIT_FAILED_MAGIC		(0x88888888)
#define MARLIN_CP_INIT_READY_MAGIC_SC2342	(0xababbaba)
#define MARLIN_CP_INIT_READY_MAGIC_UMW2631	(0xf0f0f0ff)

#endif /* __WCN_INTEGRATE_ABI_H__ */
