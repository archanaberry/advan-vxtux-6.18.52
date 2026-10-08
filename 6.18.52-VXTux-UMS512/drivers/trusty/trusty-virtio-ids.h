/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __DRIVERS_TRUSTY_TRUSTY_VIRTIO_IDS_H
#define __DRIVERS_TRUSTY_TRUSTY_VIRTIO_IDS_H

/*
 * VIRTIO_ID_TRUSTY_IPC — didefinisikan DI SINI, bukan di
 * include/uapi/linux/virtio_ids.h, karena 6.18 memakai nomor yang sama untuk
 * hal lain.
 *
 * NILAI 13 TIDAK BOLEH DIUBAH. Angkanya bukan pilihan kita: nomor device
 * datang dari secure world lewat deskriptor fw_rsc_vdev, dan driver ini
 * mencocokkan persis angka itu --
 *   drivers/trusty/trusty-virtio.c:414  tvdev->vdev.id.device = vdev_descr->id;
 *   drivers/trusty/trusty-ipc.c:1910    { VIRTIO_ID_TRUSTY_IPC, VIRTIO_DEV_ANY_ID }
 * Jadi nilainya adalah ABI dengan TOS, bukan konstanta lokal bebas.
 *
 * ⚠ TABRAKAN YANG DIKETAHUI DAN BELUM DITUTUP. Di donor 5.4 yang diport, 13
 * hanya berarti TRUSTY_IPC. Di 6.18, include/uapi/linux/virtio_ids.h:44 sudah
 * memakai 13 untuk VIRTIO_ID_MEMORY_BALLOON. Akibatnya, bila
 * CONFIG_VIRTIO_BALLOON aktif, device virtio milik trusty (id 13) akan
 * COCOK dengan dua driver sekaligus (virtio_balloon dan tipc_virtio_driver) dan
 * bus virtio hanya akan mengikatkan SATU pemenang -- siapa yang menang
 * bergantung urutan registrasi, bukan pada niat kode. Yang menang bukan jaminan
 * yang benar, dan salah pemenang berarti tipc_chan_* tidak pernah terdaftar.
 *
 * STATUS: device_artifacts/config-6.18-vxtux:8333 memang berisi
 * CONFIG_VIRTIO_BALLOON=y. Entri ini dicatat sebagai risiko di ledger
 * (baris ion_ipc_trusty) dan di docs/; TIDAK ditutup di sini karena
 * mematikannya mengubah konfigurasi device, dan itu keputusan pemilik build.
 * Cara memverifikasinya nanti (butuh build + boot): cek bahwa
 * /sys/bus/virtio/drivers/trusty-ipc/ berisi device, BUKAN
 * /sys/bus/virtio/drivers/virtio_balloon/.
 */
#define VIRTIO_ID_TRUSTY_IPC	13

#endif /* __DRIVERS_TRUSTY_TRUSTY_VIRTIO_IDS_H */
