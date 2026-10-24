# RE `sprd_iq.ko` — I/Q capture path (mode pabrik)

Blob: `donors/stock_vendor_modules_all/sprd_iq.ko` (32 KB, kernel 5.4.210).
Disassembly: [evidence-20261023/sprd_iq.objdump.txt](evidence-20261023/sprd_iq.objdump.txt)
(miring, `adrp` semua `#0`) dan
[evidence-20261023/sprd_iq.resolved.txt](evidence-20261023/sprd_iq.resolved.txt)
(relokasi `adrp`/`add`/`ldst` diterapkan seperti linker, jadi `.bss`/`.rodata`
menunjuk offset sebenarnya).

Status: **diport** — `drivers/vxtux/soc/iq/sprd_iq.c`, Kconfig `SPRD_IQ`,
`CONFIG_SPRD_IQ=m` di `enhanced_config.cfg`.

## 0. Kenapa dua dump

Dump miring memperlihatkan `adrp x8, #0 ; -> sym#134+0x38` untuk setiap akses
global, karena relokasi `ADR_PREL_PG_HI21` belum diterapkan dan nama
`sym#134` hanya hasil terkaan alat. Itu yang dulu memunculkan teori "layout
state tidak konsisten: sebagian akses berbasis `.bss+0x08`, sebagian
`.bss+0x10`". Teori itu **salah**.

Relokasi sebenarnya ada di `.rela.text.unlikely.__cfi_check_fail`, dan
addend-nya absolut terhadap seksinya (`.bss+0x38`, `.rodata+0x247`, dst).
Dengan `.symtab`: `iq_size` di `.bss+0x00` (4 B), `iq_base` di `.bss+0x04`
(4 B), `iq` di `.bss+0x08` (88 B). Setelah offset itu dipetakan, seluruh
akses konsisten (lihat §2) — termasuk `x20 = &iq+8` di awal probe, yang
hanya berarti field yang diakses memang `base`/`size`/`va` di `iq+0x08..`.

## 1. Simbol dan tata letak

| Simbol | Seksi | Ukuran | Isi |
|---|---|---|---|
| `iq_size` | .bss+0x00 | 4 | module_param `uint`, perm 0, tidak pernah dibaca |
| `iq_base` | .bss+0x04 | 4 | module_param `uint`, perm 0, tidak pernah dibaca |
| `iq` | .bss+0x08 | 88 | state driver |
| `sprd_iq_probe.__key` | .bss+0x60 | 8 | kunci lockdep untuk `&iq.wait` |
| `iq_driver` | .data+0x000 | 240 | `platform_driver` |
| `iq_mem_dev` | .data+0x0f0 | 80 | `miscdevice`, `minor = 0xff`, `name = "iq_mem"` |
| `iq_mem_fops` | .rodata+0x598 | 288 | `file_operations` |
| `sprd_iq_ids` | .rodata+0x408 | 400 | `of_device_id`, `compatible = "sprd,iq"` |

Fungsi (15): `init_module` 80 B, `in_iqmode` 152, `sprd_iq_probe` 840,
`sprd_iq_remove` 80, `iq_mem_poll` 260, `iq_mem_ioctl` 768,
`iq_mem_nocache_mmap` 188, `iq_mem_open` 48, `iq_mem_release` 48,
`_copy_to_user` 292, `_copy_from_user` 348, `sprd_iq_complete` 204,
`sprd_iq_thread` 652, `__cfi_check` 436.

`file_operations`: `.poll`, `.unlocked_ioctl`, `.mmap`, `.open`, `.release`
(tanpa `read`/`write`), `.owner = __this_module`.
`iq_driver`: `.probe`/`.remove`, `.driver.name = "sprd-iq"`,
`.driver.of_match_table = sprd_iq_ids`, tanpa `.id_table`.

`in_iqmode()` adalah **dead code**: tidak ada pemanggil di dalam modul, isinya
diduplikasi inline di awal probe. Di port hanya versi inline yang dipakai.

## 2. Tata letak state `iq` (offset pasti)

| Offset | Tipe | Isi |
|---|---|---|
| +0x00 | u32 | tidak pernah disentuh blob |
| +0x04 | u32 | `mode` (0/1/2) |
| +0x08 | u64 | `base` — `res.start` dari `sprd,region` |
| +0x10 | u64 | `size` — `resource_size(res)` (`end - start + 1`) |
| +0x18 | u32 | `offs` — `sprd,mapping-offs` (opsional, 0 bila tidak ada) |
| +0x20 | ptr | `va` hasil `shmem_ram_vmap_nocache_ex(5, 0, base, size)` |
| +0x30 | ptr | `chan` — `kzalloc(0x18)`, lihat di bawah |
| +0x38 | ptr | `thread` — `struct task_struct *`, argumen `iq_thread` |
| +0x40 | 24 B | `wait_queue_head_t`, nama lockdep `"&iq.wait"` |

`88 = 0x58` pas: `0x40 + sizeof(wait_queue_head_t)`.

`chan` (24 B, `kzalloc`, bukan bagian statis):

| Offset | Isi |
|---|---|
| +0x00 | `head` = `va` (deskriptor kepala) |
| +0x08 | `tail` = `va + size - 0x10` |
| +0x10 | `status` = `va + 0x10` — hanya diisi di `SET_MODE 2` |

`base`/`size` memakai jalur DT bila `base == 0`; kalau tidak, probe melewati
pembacaan DT. Parameter modul `iq_base`/`iq_size` **tidak** masuk ke state
(hanya ditulis di akhir probe sebagai informasi dengan perm 0), jadi
`insmod iq_base=...` tidak berpengaruh — di port parameternya dibuang.

## 3. Gate: bootargs, bukan DT

Awal probe (dan `in_iqmode()` yang tidak terpakai):

1. `of_find_node_opts_by_path("/chosen", NULL)`;
2. `of_property_read_string(chosen, "bootargs", &s)`;
3. `strstr(s, "androidboot.mode=iq")`.

- `/chosen` tidak ada atau `bootargs` tidak bisa dibaca → `pr_err`
  **"Can't not parse bootargs"**, `-ENODEV`;
- string tidak cocok → `pr_info` **"Not in iq mode"**, `-ENODEV`;
- cocok → `pr_info` **"In iq mode"**, lanjut.

Jadi driver ini hanya hidup di mode pabrik; di boot normal probe gagal
sebelum menyentuh memori.

## 4. Region memori dan mailbox

```
va + 0x000   struct iq_pkt  head      (magic, offset, size, 4 byte pad)
va + 0x010   status block   140 byte  (playback mailbox, opaque)
va + 0x100   payload pertama
va + size-0x10  struct iq_pkt  tail
```

- `sprd,region` (phandle) → `of_address_to_resource()` → `base`, `size`.
  `_dev_info "iq base: 0x%lx, iq size: 0x%lx"`.
- `sprd,mapping-offs` opsional → `offs`; `_dev_info "iq mapping_offs: 0x%x"`.
- Pemetaan lewat SIPC: `shmem_ram_vmap_nocache_ex(SIPC_ID_PSCP /*5*/, 0,
  base, size)`, dilepas dengan `shmem_ram_unmap_ex(5, 0, va)`.
- Probe menulis magic `0x5a5a5a5a` ke `head->magic` dan `tail->magic`.
- `tail = va + size - 0x10` (**tanpa** `offs`; versi lama dokumen ini salah).

Magic yang dipakai:

| Nilai | Byte | Arti |
|---|---|---|
| `0x81005a00` | — | slot bebas |
| `0x81815a5a` | — | slot terisi, thread harus mengonsumsi |
| `0x5a5a5a5a` | — | tanda probe di kedua slot |
| `0x424f504e` | `"NPOB"` | `SET_MODE 2` mengisi `head` |
| `0x44435049` | `"IPCD"` | status playback: permintaan |
| `0x424c434b` | `"KCLB"` | status playback: ack, ditulis ke `head->magic` |
| `0x4441504d` | `"MPAD"` | modem memindahkan data (dibaca thread) |
| `0x504d4441` | `"ADMP"` | AP sudah meng-ack (ditulis thread, ditunggu `poll`) |

## 5. `ioctl`

Setiap panggilan `pr_info("enter ioctl cmd = 0x%x")`. Guard: `chan`, `head`,
atau `tail` kosong → `-EPERM` **tanpa pesan**. Perintah tak dikenal → `0`
(bukan `-ENODEV`).

| cmd | Perilaku |
|---|---|
| `0` | Slot pertama yang ber-magic `0x81005a00` (kepala dulu, lalu ekor). `buf.offset = slot.offset - (u32)base - offs`, `buf.size = slot.size`; keduanya sibuk → `{0,0}`. `copy_to_user(arg, &buf, 8)` gagal → `"copy iq buf info to user space failed."`, `-EFAULT`. |
| `1` | Wajib `mode == 2`, kalau tidak `"current mode is not playback mode"`, `-EINVAL`. `copy_to_user(arg, status, 0x8c)`; gagal → `"copy iq playback data info to user space failed."`, `-EFAULT`. |
| `0x80` | `arg > 2` → `"iq ch type invalid"`, `-EINVAL`. `mode = arg`. `arg == 1`: bila kepala **atau** ekor ber-magic `0x81005a00` → `__wake_up(&iq.wait)`. `arg == 2`: `chan->status = va + 0x10`, `head->offset = (u32)base + 0x100`, `head->magic = 0x424f504e`. `arg == 0`: hanya set mode. |
| `0x82` | `head->magic == 0x81005a00` → `0x81815a5a`; kalau tidak, `tail` dengan cara sama. Selalu `0`. |
| `0x83` | Wajib `mode == 2`. `copy_from_user(status, arg, 0x8c)` gagal → `"copy iq playback data info from user space failed."`, `-EFAULT`. `head->size = status[2]`; bila `status[0] == "IPCD"` → `head->magic = "KCLB"`. |

Catatan: `SET_MODE 2` menulis **kepala**, bukan ekor (versi lama dokumen ini
salah). `0x8c` = 140 byte, struktur playback milik tool vendor; driver tidak
mem-parsing-nya selain `status[0]` dan `status[2]`.

## 6. `poll`

`poll_wait(filp, &iq.wait, wait)` lalu:

- `chan`/`head`/`tail` kosong → `EPOLLERR`;
- `mode == 2` dan `status[0] == 0x504d4441` (`"ADMP"`) → `pr_err`
  `"I/Q playback status DATA_AP_MOVING"` lalu `EPOLLIN|EPOLLRDNORM`; magic
  lain → `0`;
- `mode == 1` dan kepala **atau** ekor ber-magic `0x81005a00` →
  `EPOLLIN|EPOLLRDNORM`;
- selain itu `0`.

Tidak ada `POLLOUT`.

## 7. `mmap`

- `base == 0` → `"get iq mem info failed"`, `-EAGAIN`;
- `vm_end - vm_start > size` → `"map size to big, exceed maxsize."`, `-EAGAIN`;
- `pfn = base >> PAGE_SHIFT` — **alamat fisik**, bukan `va` (versi lama
  dokumen ini salah); `vm_page_prot` dibersihkan bit 2..4 lalu di-OR
  `0x60000000000000` (= `PROT_NORMAL_NC` arm64), `remap_pfn_range()`;
- sukses → `pr_info("iq mmap %x,%x,%x")`; gagal → `"remap_pfn_range failed"`,
  `-EAGAIN`.

Jadi userspace melihat region fisik mulai `base`, tanpa `offs`.

## 8. Thread `iq_thread`

Dibuat `kthread_run(iq_thread, &iq, "iq_thread")`;
`sched_setscheduler(current, SCHED_FIFO, {.prio = 0x50})`. Loop:

1. `kthread_should_stop()` → keluar;
2. `mode == 2` dan `status[0] == 0x4441504d` → tulis `0x504d4441`,
   `__wake_up(&iq.wait)`;
3. kepala ber-magic `0x81815a5a` → tulis `0x81005a00`, lalu:
   - `mode == 1` → `__wake_up()` dan ulang;
   - `mode == 0` → `pr_info("iq write 0x%x, 0x%x", offset - offs, size)` lalu
     kirim `__va(offset - offs + terkirim)` lewat `vser_pass_user_write()`
     dalam potongan maksimum `0xd0000`, `msleep(200)` bila gagal (tidak
     memajukan `terkirim`);
   - `mode` lain → ulang tanpa aksi;
4. kalau kepala bukan `0x81815a5a`, langkah 3 diulang untuk ekor;
5. tidak ada yang cocok → `pr_info("ch: %d, hand1: 0x%x, flag2: 0x%x")` +
   `msleep(1500)`.

## 9. Callback `sprd_iq_complete`

Didaftarkan `kernel_vser_register_callback(sprd_iq_complete, 0)`.
Bila `mode != 0` → langsung kembali. `pr_info("complete 0x%p, 0x%x")`; lalu
bila `buf + len == __va(head->offset - offs + head->size)` → `head->magic =
0x81815a5a`, dan sama untuk `tail`.

## 10. Bug blob yang tidak ditiru

1. `sprd_iq_remove()` melepas mapping hanya bila `va == 0` (kondisi terbalik),
   tidak memanggil `kthread_stop()`, dan thread tetap berjalan setelah `kfree`.
   Port: stop thread, deregister callback, unmap, lalu `kfree`.
2. `probe` memanggil `misc_register()` sebelum region di-map; kegagalan
   berikutnya harus men-`misc_deregister()`. Port mendaftarkan device setelah
   mapping siap.
3. `chan->status` baru diisi di `SET_MODE 2`, padahal `poll()` mode 2
   membacanya. Port mengisi `va + 0x10` sejak probe.
4. `head`/`tail`/`offset`/`size` dari region dipakai apa adanya sebagai alamat
   `__va()`. Port menolak deskriptor di luar `[base, base+size)` sebelum
   mengirim (`iq_pkt_valid()`); panjang 0 tetap diterima seperti blob.
   Probe juga menolak region yang tidak cukup untuk dua deskriptor
   (`size <= 0x10 + sizeof(struct iq_pkt)`), memakai pesan blob sendiri
   `"invalid iq_base(0x%llx)."` yang di blob tidak pernah tercetak.
5. `mmap` memakai `remap_pfn_range()` dengan `vm_page_prot` yang diutak-atik
   manual. Port memakai `pgprot_writecombine()` + `VM_IO|VM_PFNMAP`.

## 11. Node DT belum ada di DTS port

Di DT stok yang benar-benar dipakai device ([STOCK-DT-NODES.md](STOCK-DT-NODES.md))
kedua node ini ada:

```dts
reserved-memory {
	iq_mem_reserved: iq-mem@90000000 {
		compatible = "sprd,iq-mem";
		reg = <0x0 0x90000000 0x0 0x4000000>;   /* 64 MB */
		/* tanpa no-map: driver memakai __va() pada region ini */
	};
};

/ {
	sprd-iq {
		compatible = "sprd,iq";
		sprd,region = <&iq_mem_reserved>;
		sprd,mapping-offs = <0x0>;
	};
};
```

`arch/arm64/boot/dts/sprd/ums512-1h10-vxtux.dts` **belum** memuat keduanya, dan
`0x90000000..0x94000000` di DTS itu masih bagian dari `memory@80000000`
(`reg = <0x0 0x80000000 0x0 0x80000000>`) serta tidak dipakai node lain.
Artinya dengan DTB hasil port ini driver **tidak akan probe** walau bootargs
memuat `androidboot.mode=iq`; ia baru bisa jalan dengan DTB stok, atau setelah
node di atas ditambahkan (konsekuensinya 64 MB RAM direservasi di setiap boot,
sama seperti firmware stok).

## 12. Yang belum diverifikasi

Build belum dijalankan: codespace `reimagined-palm-tree-g49xvp9vxx652ww5q`
sudah tidak ada, satu-satunya codespace tersisa
(`solid-tribble-9rjjxvjqq5pcpq5g`) menolak start dengan HTTP 402 (billing),
dan tidak ada WSL/toolchain lokal — `make olddefconfig` di Windows gagal di
`scripts/kconfig` (mingw tidak punya `getline`/`mkdir(mode)`).

Yang bisa diuji tanpa build: pemetaan relokasi di
[sprd_iq.resolved.txt](evidence-20261023/sprd_iq.resolved.txt) dan
pencocokan offset state di §2. Perilaku mode IQ (vmap SIPC, PFN, handshake
mailbox) hanya bisa diuji di perangkat dengan `androidboot.mode=iq`.
