/* SPDX-License-Identifier: GPL-2.0
 *
 * headset-618-compat.h — shim tambahan untuk port headset/codec sc2730
 * (5.4 vendor fork -> 6.18 mainline). Hanya untuk berkas di codec_sc2730/.
 *
 * Pemetaan (semua terverifikasi di ktree618):
 *   wakeup_source_init(ws, name)      -> simpan pointer hasil
 *                                        wakeup_source_register(dev?, name).
 *                                        ws struct tak dipakai by-value lagi;
 *                                        hdst->..._wakelock jadi pointer
 *                                        (lihat sprd-headset-2730.h).
 *   gpio_set_debounce(gpio, t)        -> (NOP terkendali) API legacy int-space
 *                                        dihapus; debounce dtb kini milik
 *                                        gpiod/driver. Nilai vendor tetap
 *                                        dicatat via pr_debug supaya mudah
 *                                        dipindah ke DTS nanti.
 *   of_get_gpio_flags(np, i, NULL)    -> of_get_named_gpio(np, i) + macro
 *                                        dummy index (flags tak dipakai).
 *   of_get_named_gpio_flags(np,p,i,&f) -> TIDAK di-shim: pembacaan flags
 *                                        native (vxtux_of_get_named_gpio_
 *                                        flags) kini hidup di
 *                                        sprd-headset-2730.h.
 *   devm_gpio_request(dev, g, l)      -> SUDAH tidak dipakai: kedua
 *                                        call-site memakai
 *                                        devm_gpio_request_one(dev, g,
 *                                        GPIOF_IN, l) langsung.
 *   snd_soc_card_jack_new(6 args)     -> snd_soc_card_jack_new_pins(card,
 *                                        id, type, jack, NULL, 0).
 *   audio_class->p->subsys.kobj       -> &audio_class->p — class p kini
 *                                        priv, jadi link dibuat langsung ke
 *                                        class kobject.
 *   dai->codec                        -> dai->component (ASoC >= 4.17).
 */
#ifndef _VXTUX_HEADSET_618_COMPAT_H
#define _VXTUX_HEADSET_618_COMPAT_H

#include <linux/pm_wakeup.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>

/* ---- wakeup source ----
 * 6.18: wakeup_source_register() mengembalikan pointer dan wakeup_source
 * tidak lagi bisa dipakai by-value. Makro ini dipakai dengan alamat member
 * struct lama (&hdst->x_wakelock); ekspansi mengambil alamat member itu
 * (bertipe struct wakeup_source **), mengisi dengan pointer hasil register,
 * lalu helper __pm_* di bawah dereferensi kembali. Member struct sudah
 * diubah jadi pointer di sprd-headset-2730.h — &member == member. */
#define wakeup_source_init(ws, name) \
	({ *(struct wakeup_source **)(ws) = wakeup_source_register(NULL, name); \
	   NULL; })

/* ---- debounce int-space dihapus di 6.18 ---- */
#define gpio_set_debounce(gpio, t) \
	do { \
		pr_debug("%s: debounce %u us (gpio %d) pindah ke DTS/gpiod\n", \
			 __func__, (unsigned)(t), (int)(gpio)); \
	} while (0)

/* ---- of_gpio: API flags-based dihapus di 6.18 ----
 * of_get_gpio_flags(np, index, flags) di fork dipakai dengan flags=NULL
 * (situs hanya butuh nomor GPIO dari daftar "gpio-names" terindeks). 6.18
 * hanya menyediakan of_get_named_gpio(np, list_name, index); makro ini
 * meneruskan index apa adanya. Enum of_gpio_flags/OF_GPIO_ACTIVE_LOW tidak
 * lagi dideklarasikan di mana pun: sprd-headset-sc2730.c memakai
 * vxtux_of_get_named_gpio_flags() + GPIO_ACTIVE_LOW. */
#define of_get_gpio_flags(np, index, flags) \
	of_get_named_gpio((np), "gpio-names", (index))

/* ---- jack: versi 6 argumen = versi pins ---- */
#define snd_soc_card_jack_new(card, id, type, jack, pins, npins) \
	snd_soc_card_jack_new_pins((card), (id), (type), (jack), \
				   (struct snd_soc_jack_pin *)(pins), (npins))

#endif /* _VXTUX_HEADSET_618_COMPAT_H */
