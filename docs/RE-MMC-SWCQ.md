# RE `mmc_swcq.ko` — Unisoc software command queue (SW-CQ) untuk eMMC

Blob: `donors/stock_vendor_modules_all/mmc_swcq.ko` (72.670 B, 40 fungsi,
kernel 5.4.210). Blob pembanding yang memuat pemanggilnya:
`donors/stock_vendor_modules_all/sdhci-sprd.ko`.

Sumber GPL vendor (donor utama port):
[donors/gpl/mmc_swcq.c](../donors/gpl/mmc_swcq.c) (2.413 baris) +
[donors/gpl/mmc_swcq.h](../donors/gpl/mmc_swcq.h). Donor ini bukan 5.4 mentah:
`struct mmc_cqe_ops` sudah memakai nama 6.x (`cqe_enable`, `cqe_request`,
`cqe_post_req`, `cqe_wait_for_idle`, `cqe_recovery_start/finish`,
`cqe_timeout`) plus dua stash `android_kabi_reserved1/2` khas GKI, jadi
sebagian besar port-nya hanya rename.

Status: **diport** — `drivers/mmc/host/mmc_swcq.c` + `mmc_swcq.h`,
Kconfig `MMC_SWCQ` (`drivers/mmc/host/Kconfig`),
`obj-$(CONFIG_MMC_SWCQ) += mmc_swcq.o`, `CONFIG_MMC_SWCQ=m` di
`enhanced_config.cfg`.

## 1. Fakta blob

| Item | Nilai |
|---|---|
| ukuran | 72.670 B |
| `MODULE_DESCRIPTION` | `MMC Host Software Queue support` |
| `MODULE_LICENSE` | `GPL v2` |
| EXPORT | `dbg_add_host_log`, `sdhci_sprd_timeout_register`, `mmc_swcq_finalize_request`, `mmc_swcq_init`, `mmc_swcq_suspend`, `mmc_swcq_resume` |
| simbol | 62, dominan `blk_mq_*` (`__blk_mq_end_request`, `blk_mq_end_request`, `blk_mq_requeue_request`, `blk_mq_run_hw_queues`, `blk_mq_complete_request`) + `mmc_*` |
| struct state | `struct mmc_swcq`, ±45 KB (`sizeof` di blob: `0xb018`), termasuk `cmd_history[400]` |

`sdhci-sprd.ko` memanggil `mmc_swcq_init()`/`suspend`/`resume`; IRQ host
memanggil `mmc_wait_cmdq_done()` (lihat §3).

## 2. Pemilihan eMMC vs SD — `mmc_hsq_swcq_init` @ `sdhci-sprd.ko+0x4af0`

Disassembly (`donors/blobre.py donors/stock_vendor_modules_all/sdhci-sprd.ko
dis mmc_hsq_swcq_init`):

```
4b00: ldr  x8, [x0, #0x40]      ; x8 = sdhci_host->mmc
4b0c: add  x0, x1, #0x10        ; &pdev->dev
4b10: ldr  w8, [x8, #0x3f4]     ; w8 = mmc->caps2
4b04: mov  w9, #0x280000
4b14: bics wzr, w9, w8          ; (caps2 & 0x280000) == 0x280000 ?
4b18: b.ne hsq_branch
     ; eMMC: devm_kmalloc(&pdev->dev, 0xb018) -> mmc_swcq_init(swcq, host->mmc)
     ;       lalu str #0x000f4240 (1,000,000) ke swcq+0x3f60
     ; SD  : devm_kmalloc(&pdev->dev, 0x180)  -> mmc_hsq_init(hsq, host->mmc)
```

`0x280000` = `BIT(19)|BIT(21)`; di 6.18 itu
`MMC_CAP2_NO_SDIO|MMC_CAP2_NO_SD` — persis makro `HOST_IS_EMMC_TYPE()` di
`mmc_swcq.h` donor. Jadi **blob tidak memakai compatible DT**, melainkan
`caps2` yang sudah disetel probe host sebelumnya. Port mengikuti itu:
`sdhci_sprd_probe()` menyetel kedua bit untuk controller `non-removable`
(setelah `mmc_of_parse`), `mmc_hsq_swcq_init()` menguji bit yang sama.

Store `0xf4240` ke `swcq+0x3f60` tidak direplikasi: field itu tidak dibaca
di jalur mana pun donor (`poll_timeout` hanya ada di struct; `CMD13_TMO_NS`
punya konstanta sendiri `1e6` di .c) — kemungkinan sisa debug.

## 3. Pembagian kerja dengan host

| Fungsi | Pemanggil di blob | Di port 6.18 |
|---|---|---|
| `mmc_swcq_init(swcq, mmc)` | probe `sdhci-sprd` | `mmc_hsq_swcq_init()` (ext.c) |
| `mmc_swcq_suspend/resume(mmc)` | runtime PM `sdhci-sprd` | `sdhci_sprd_runtime_suspend/resume()` lewat `cqe_private` |
| `mmc_wait_cmdq_done(mrq)` | IRQ host (`sdhci-sprd`) | dipakai internal donor: `mrq->done = mmc_wait_cmdq_done` untuk CMD13'/44/45/46/47; tak perlu dipanggil dari IRQ |
| `mmc_swcq_finalize_request(mmc, mrq)` | core blob (DCMD blk-mq) | `sdhci_sprd_request_done()` |

`mmc_swcq_finalize_request()` adalah hook core vendor: di kernel vendor,
CMD44/45/46/47 milik core dikirim sebagai request blk-mq passthrough
bertanda, lalu core memanggil finalize. **Core 6.18 ini tidak punya jalur
itu**: `mmc_cqe_issue_type()` memetakan `REQ_OP_DRV_IN/OUT/DISCARD/...` ke
`MMC_ISSUE_SYNC` (jalur drv_op biasa), dan tidak ada `REQ_RAISE_BLKTAG`
maupun `blk_mq` DCMD. Konsekuensinya tak ada call-site alami; fungsi tetap
diekspor dan dipakai `sdhci_sprd_request_done()` untuk mrq SW-CQ yang selesai
(cek `mmc->cqe_private != NULL` sebagai penanda host SW-CQ, karena HSQ
menyimpan dirinya di `mmc->private`).

## 4. Delta port (donor GPL → 6.18)

| # | Donor | 6.18 | Alasan |
|---|---|---|---|
| 1 | `if (mq->use_cqe && !host->hsq_enabled)` | `if (host->cqe_enabled && ...)` | `struct mmc_queue` 6.18 tak punya `use_cqe`; core memakai kondisi ini di `mmc_issue_type()` |
| 2 | `from_timer(swcq, t, check_timer)` | `timer_container_of(...)` | rename di 6.16 |
| 3 | `mmc_swcq_ops.android_kabi_reserved1/2 = ...` | dihapus | 6.18 `struct mmc_cqe_ops` tak punya slot kABI; `mmc_swcq_is_busy()` diberi `__maybe_unused` |
| 4 | `mq->cqe_busy &= ~MMC_CQE_QUEUE_FULL` | dihapus | 6.18 hanya menyisakan `MMC_CQE_DCMD_BUSY` |
| 5 | `proc_create(..., file_operations*)` | `struct proc_ops` | API procfs ≥5.6 |
| 6 | (tak ada) `host->cqe_qdepth` | diisi `EMMC_MAX_QUEUE_DEPTH` di `mmc_swcq_enable()` | `mmc_init_queue()` 6.18: `min(cmd_ext_csd.cmdq_depth, host->cqe_qdepth)` → 0 kalau tak diisi |
| 7 | `#include <linux/mmc/mmc_sdhci_sprd.h>` | dibuang | header privat vendor; semua yang dipakai ada di `mmc/mmc.h` |
| 8 | `struct mmc_blk_data` versi donor (ada `usage`, tanpa `kref`) | layout 6.18 dari `core/block.c` | swcq menulis `md->reset_done`; offset harus sama |

Yang **tidak** berubah: `struct mmc_swcq`, `cmdq_slot`, `swcq_node`,
`cmd_history`, mesin state SW-CQ (CMD13' → CMD44/45 → CMD46/47, mode
hsq/cmdq switch berbasis `check_cmdq_timer`, recovery `mmc_reset_for_cmdq`),
serta `mmc_cqe_ops` lain yang sudah bernama 6.x.

## 5. Verifikasi yang sudah/belum

Sudah (statis, terhadap source 6.18 di tree ini):

- semua anggota `struct mmc_cqe_ops` yang dipakai donor ada di
  `include/linux/mmc/host.h:277` dan signature-nya sama;
- semua simbol core yang dipanggil diekspor: `mmc_wait_for_cmd`,
  `mmc_post_req`, `mmc_cqe_post_req`, `mmc_cqe_request_done`,
  `mmc_request_done`, `mmc_command_done`, `mmc_start_request`,
  `mmc_cmdq_enable/disable`, `mmc_switch`, `mmc_hw_reset`, `mmc_put_card`,
  `mmc_send_status`, `mmc_poll_for_busy`;
- `q->queuedata == mq`: 6.18 menyetelnya lewat
  `blk_mq_alloc_disk(&mq->tag_set, &lim, mq)` di `mmc_alloc_disk()`;
- `request_atomic` ada di `struct mmc_host_ops` (`include/linux/mmc/host.h:175`)
  dan `dump_vendor_regs` di `struct sdhci_ops` (`drivers/mmc/host/sdhci.h:724`);
- `mmc_issue_type`/`mmc_cqe_check_busy` yang di-`extern` header donor tidak
  dipanggil .c (donor punya salinan `static swcq_mmc_*`), jadi tak perlu
  `EXPORT_SYMBOL` tambahan di `core/queue.c`.

Belum:

- build ulang lewat CI (`quick-cc.yml` target
  `drivers/mmc/host/mmc_swcq.o`, lalu `v17-build.yml`);
- boot QEMU + uji eMMC CMDQ (butuh node eMMC `non-removable` dan
  `CONFIG_MMC_SWCQ=m`).

## 6. Sisa pekerjaan yang diketahui

1. `raw_sdhci_thread_irq` di `sdhci-sprd-ext.c` masih meneruskan ke
   `sdhci_thread_irq` upstream; blob punya salinan handler sendiri yang
   "drain" SW-CQ. Untuk SW-CQ yang menyelesaikan mrq lewat `->done`
   (`mmc_wait_cmdq_done`), jalur upstream sudah memadai — perlu diuji di
   perangkat sebelum mengganti.
2. `sdhci_sprd_host_hook()` (extern di header donor) tidak punya definisi
   maupun pemanggil di donor .c; dihapus dari header port.
3. `sprd-cpufreqsw.c` (gpu volt lock / slave notify) tidak termasuk port
   SW-CQ; lihat PORT-REMAINING §4.
