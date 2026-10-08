/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020 Unisoc Inc.
 *
 * sprd_gpu_cooling.h - deklarasi publik ABI sprd_gpu_cooling.
 *
 * Dua fungsi di bawah ini adalah satu-satunya ekspor blob vendor
 * `sprd_gpu_cooling.ko` (lihat __ksymtab di blobs/re_notes/
 * sprd_gpu_cooling.sym): keduanya EXPORT_SYMBOL biasa (bukan _GPL) dan
 * dipanggil driver GPU (kbase/DDK Mali) saat probe/remove.
 *
 * Deklarasi publik wajib ada: tanpa ini kompilasi memunculkan
 * -Wmissing-prototypes untuk kedua definisinya (ditemukan build ronde-18).
 */

#ifndef __SPRD_GPU_COOLING_H__
#define __SPRD_GPU_COOLING_H__

struct devfreq;
struct device;

/**
 * create_gpu_cooling_device() - daftarkan cooling device GPU dari DT
 * @devfreq: devfreq milik GPU (dipakai of_devfreq_cooling_register_power())
 * @dev:     divalidasi non-NULL; blob vendor tidak memakainya lebih lanjut
 *
 * Menelusuri "gpu-cooling-devices" -> anak ber-`compatible =
 * "sprd,mali-power-model"`, membaca koefisien power model, lalu mendaftarkan
 * devfreq cooling untuk setiap anak (slot diindeks alias-id "gpu-cooling").
 *
 * Return: 0 sukses, negatif kode errno.
 */
int create_gpu_cooling_device(struct devfreq *devfreq, struct device *dev);

/**
 * destroy_gpu_cooling_device() - lepas semua cooling device GPU
 *
 * Mencocokkan nama node anak dengan slot yang tercatat, meng-unregister
 * cooling device-nya, lalu membebaskan tabel slot.
 *
 * Return: 0 sukses, negatif kode errno.
 */
int destroy_gpu_cooling_device(void);

#endif /* __SPRD_GPU_COOLING_H__ */
