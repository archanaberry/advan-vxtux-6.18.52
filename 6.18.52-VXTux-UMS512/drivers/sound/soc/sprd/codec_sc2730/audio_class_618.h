/* SPDX-License-Identifier: GPL-2.0
 *
 * audio_class_618.h — akses kobject kelas `audio` di 6.18.
 *
 * Fork: sysfs_create_link(&audio_class->p->subsys.kobj, ..., "earjack").
 * 6.18: anggota p dihapus dari struct class (sekarang privatin, dikelola
 * drivers/base). Satu-satunya jalur resmi adalah class_to_subsys(), yang
 * TIDAK diekspor ke modul (lihat drivers/base/class.c) — jadi pemanggilan
 * langsung tidak akan link untuk modul, tetapi unit built-in tetap bisa
 * memanggilnya lewat deklarasi sendiri.
 *
 * ASoC port ini dibangun built-in (=y), jadi kita boleh mendeklarasikan
 * prototipe class_to_subsys() sendiri (persis seperti di drivers/base/base.h)
 * dan memanggilnya. subsys_get() dilakukan fungsi itu sendiri; pasangan
 * put() kita lakukan setelah create_link.
 */
#ifndef _VXTUX_AUDIO_CLASS_618_H
#define _VXTUX_AUDIO_CLASS_618_H

#include <linux/kobject.h>

/* 6.18: drivers/base/base.h (subsys_private penuh) tidak untuk driver.
 * Kita hanya butuh pointer kobject kelas untuk sysfs_create_link. Link ini
 * dibuat saat probe dan HIDUP selama kelas audio hidup; class_to_subsys()
 * mengambil referensi yang dilepas subsys_put() — keduanya didefinisikan di
 * drivers/base (class_to_subsys tidak diekspor, tapi port dibangun built-in
 * sehingga tetap bisa link). Untuk menghindari menarik subsys_put() yang juga
 * tidak diekspor, referensi TIDAK dilepas: kelas audio tidak pernah
 * unregister sebelum mati sistem (fork: module_exit class_unregister; port
 * built-in: tak pernah). */
struct subsys_private {
	struct kset subsys;
};

struct subsys_private *class_to_subsys(const struct class *class);

#define vxtux_class_kobj(cls)						\
	({								\
		struct subsys_private *_sp = class_to_subsys((cls));	\
		struct kobject *_k = _sp ? &_sp->subsys.kobj : NULL;	\
		if (!_k)							\
			pr_err("%s: kobject kelas tidak ditemukan\n",	\
			       __func__);					\
		_k;							\
	})

#endif /* _VXTUX_AUDIO_CLASS_618_H */
