# RE `sprd_iq.ko` — I/Q capture path (mode pabrik)

Blob: `donors/stock_vendor_modules_all/sprd_iq.ko` (32 KB, kernel 5.4.210).
Disassembly: [evidence-20261023/sprd_iq.objdump.txt](evidence-20261023/sprd_iq.objdump.txt)
(per fungsi, dari `donors/blobre.py … dis <fungsi>`).

Status: **analisis selesai, driver belum ditulis** — lihat "Yang masih gelap".

## 1. Simbol dan tata letak global

| Simbol | Seksi | Ukuran | Isi |
|---|---|---|---|
| `iq_size` | .bss+0x00 | 4 | module_param (`iq_sizetype`) |
| `iq_base` | .bss+0x04 | 4 | module_param (`iq_basetype`) |
| `iq` | .bss+0x08 | 88 | state driver |
| `sprd_iq_probe.__key` | .bss+0x60 | 8 | kunci lockdep untuk `&iq.wait` |
| `iq_driver` | .data+0x000 | 240 | `platform_driver` |
| `iq_mem_dev` | .data+0x0f0 | 80 | `miscdevice` |
| `iq_mem_fops` | .rodata+0x598 | 288 | `file_operations` |
| `sprd_iq_ids` | .rodata+0x408 | 400 | `of_device_id`, entri pertama `compatible = "sprd,iq"` |

Fungsi (15): `init_module` 80 B, `in_iqmode` 152, `sprd_iq_probe` 840,
`sprd_iq_remove` 80, `iq_mem_poll` 260, `iq_mem_ioctl` 768,
`iq_mem_nocache_mmap` 188, `iq_mem_open` 48, `iq_mem_release` 48,
`_copy_to_user` 292, `_copy_from_user` 348, `sprd_iq_complete` 204,
`sprd_iq_thread` 652, `__cfi_check` 436.

`file_operations` (offset 5.4 arm64 diverifikasi lewat stub CFI di
`.text+0x11b8..`): `.poll` = `iq_mem_poll`, `.unlocked_ioctl` = `iq_mem_ioctl`,
`.mmap` = `iq_mem_nocache_mmap`, `.open` = `iq_mem_open`,
`.release` = `iq_mem_release`, `.owner` = `__this_module`. Tidak ada
`read`/`write`; seluruh I/O lewat `ioctl` + `mmap`.

`iq_driver`: `.probe` = `sprd_iq_probe`, `.remove` = `sprd_iq_remove`,
`.driver.name` = `"sprd-iq"`, `.driver.of_match_table` = `sprd_iq_ids`,
`.id_table` tidak ada. Nama misc device `"iq_mem"`, `mode = 0xff`
(`MISC_DYNAMIC_MINOR`).

## 2. Gate: bootargs, bukan DT

`in_iqmode()` dan `sprd_iq_probe()` sama-sama:

1. `of_find_node_opts_by_path("/chosen", NULL)`;
2. `of_property_read_string(chosen, "bootargs", &s)`;
3. `strstr(s, "androidboot.mode=iq")`.

Di probe: bootargs tidak ada → `printk` **"Can't not parse bootargs"** lalu
`-ENODEV`; string tidak cocok → `printk` **"Not in iq mode"** lalu `-ENODEV`
(probe tidak pernah `printk` "In iq mode" — itu milik `in_iqmode`).

Artinya driver ini **hanya hidup di mode pabrik** dan aman untuk perangkat
normal: tanpa `androidboot.mode=iq` probe gagal sebelum menyentuh memori.

## 3. Region memori

Dua jalur di probe:

- jalur DT (dipakai bila `iq` state belum berisi pointer region):
  `of_parse_phandle(dev->of_node, "sprd,region", 0)` →
  `of_address_to_resource()` → `iq.base = res.start`,
  `iq.size = res.end - res.start + 1` (perhatikan `+ 1`).
  `_dev_info` mencetak `"iq base: 0x%lx, iq size: 0x%lx"`.
- jalur fallback: memakai nilai yang sudah ada di state (dari module_param
  `iq_base`/`iq_size`), lalu tetap membaca
  `of_property_read_variable_u32_array(np, "sprd,mapping-offs", &off, 1, NULL)`
  (opsional). `_dev_info` mencetak `"iq mapping_offs: 0x%x"`.
- Akhir probe: `iq_base = base + mapping_offs` (u32, hanya 32 bit bawah) dan
  `iq_size = size`.

Pemetaan tidak memakai `ioremap`/`memremap`, melainkan API SIPC:

```c
iq.va = shmem_ram_vmap_nocache_ex(5 /* dst */, 0 /* smem */, base, size);
...
shmem_ram_unmap_ex(5, 0, iq.va);
```

Keduanya **sudah ada** di tree 6.18:
`include/linux/soc/sprd/sipc.h` (deklarasi) dan
`drivers/soc/sprd/sipc/smem.c` (implementasi, termasuk varian `_ex`).

Setelah map, probe menulis magic `0x5a5a5a5a` di **dua slot 16-byte**:
slot kepala di `va + 0` dan slot ekor di `va + size + mapping_offs - 0x10`
(dihitung sebagai `iq.size + iq.mapping_offs - 0x10`, lalu disimpan ke
pointer kedua di state). Kedua slot itu yang dipakai sebagai mailbox
status antara AP, modem, dan userspace.

## 4. `ioctl` — perintah yang terbaca

Semua cabang masuk lewat `printk("enter ioctl cmd = 0x%x", cmd)`. Guard awal:
bila pointer region atau ukurannya nol → `return -1` (`-EPERM`) tanpa pesan.

| cmd | Perilaku |
|---|---|
| `0` | Baca slot kepal/ekor yang ber-magic `0x81005a00`, hitung `offset = slot.offset - iq_base - mapping_offs`, ambil `len = slot.len`, `copy_to_user(arg, &{offset,len}, 8)`. Gagal salin → `"copy iq buf info to user space failed."`, `-EFAULT`. |
| `1` | `copy_to_user(arg, playback, 0x8c)`; gagal → `"copy iq playback data info to user space failed."`, `-EFAULT`. |
| `0x80` | Set mode kerja: `arg > 2` → `printk("param error")`, `-EINVAL`. `mode = arg`; untuk `arg == 2` state diarahkan ke `va + 0x10` lalu slot ditandai magic `"PNOB"`-family (`0x424f4e50`) dengan field panjang `+0x100`; untuk `arg == 1` hanya `__wake_up(&iq.wait, 1, 1, NULL)`. Mode bukan playback → `"current mode is not playback mode"`. |
| `0x82` | Bila magic slot `0x81005a00` → tulis `0x81815a5a` (bit "data siap" untuk thread). Slot kedua diperiksa dengan cara sama. |
| `0x83` | Wajib `mode == 2`, kalau tidak `"current mode is not playback mode"`. `copy_from_user(playback, arg, 0x8c)`, lalu `slot.len = playback.len`; bila magic playback `"IPCD"`/`"KBLC"` → tulis magic `"KBLC"` ke slot. Gagal salin → `-EFAULT`. |
| lain | `return 0` (tanpa aksi). |

`0x8c` = 140 byte: struktur playback yang dipakai tool IQ vendorside
(disalin bolak-balik, tidak pernah di-parse driver).

## 5. `poll`

`poll_wait()` lalu cek state:

- pointer region kosong → `POLLERR` (`0x8`);
- `mode == 2` dan `slot.magic == 0x4441504d` (`"MPAD"` — "DATA_AP_MOVING")
  → `printk("I/Q playback status DATA_AP_MOVING")` lalu `POLLIN|POLLRDNORM`
  (`0x41`);
- `mode == 1` dan `slot.magic == 0x81005a00` → `0x41`;
- selain itu `0` (tidur lagi).

Tidak ada `POLLOUT`, jadi device ini hanya bisa di-`poll` untuk "ada data".

## 6. `mmap`

- state/region kosong → `printk("get iq mem info failed")`, `-EAGAIN`;
- `vma->vm_end - vma->vm_start > iq_size` → `"map size to big, exceed maxsize."`,
  `-EAGAIN`;
- `vma->vm_page_prot` dibersihkan bit 2..4 lalu di-OR `0x60000000000000`
  (atribut normal-non-cache = `PROT_NORMAL_NC` di pgprot arm64),
  `remap_pfn_range(vma, start, va >> PAGE_SHIFT, len, prot)`;
- sukses `printk("iq mmap %x,%x,%x")`; gagal → `"remap_pfn_range failed"`,
  `-EAGAIN`.

Perhatikan: `remap_pfn_range` dipanggil dengan `pfn` dari **virtual address**
hasil `shmem_ram_vmap_nocache_ex`, yaitu pola khas blob vendor 5.4 untuk
region yang di-map linear; pada 6.18 perlu dikonversi ke PFN fisik
(`__pa(iq.va) >> PAGE_SHIFT`) agar tidak salah halaman.

## 7. Thread `iq_thread`

Dibuat `kthread_create_on_node(iq_thread, &iq, -1, "iq_thread")` +
`wake_up_process`; `sched_setscheduler(current, SCHED_FIFO, {.prio = 0x50})`.
Loop:

1. `kthread_should_stop()` → keluar;
2. `mode == 2` dan slot magic `"MPAD"` → tulis `"MPAD"` balik lalu
   `__wake_up(&iq.wait, 1, 1, NULL)`;
3. slot kepala magic `0x81815a5a` → tulis `0x81005a00`; lalu:
   - `mode == 1` → `__wake_up()` dan lanjut;
   - `mode == 0` → `printk("iq write 0x%x, 0x%x", off - iq_base, len)` lalu
     kirim payload ke USB lewat `vser_pass_user_write(phys_to_virt(off), n)`
     dalam potongan maksimum `0xd0000` byte, `msleep(200)` bila gagal;
   - mode lain → `printk("ch: %d, hand1: 0x%x, flag2: 0x%x")` +
     `msleep(1500)`;
4. slot ekor dibuat dengan cara yang sama (dua salinan blok kode, satu untuk
   kepala satu untuk ekor).

Callback `sprd_iq_complete(addr, len)` didaftarkan lewat
`kernel_vser_register_callback()`: bila `mode != 0` langsung kembali;
mencetak `"sprd-iq: complete 0x%p, 0x%x"` lalu menulis magic `0x81815a5a`
bila `addr` sama dengan alamat fisik slot kepala atau ekor
(`phys = (slot.offset - iq_base + slot.len) - memstart_addr | 0xffffff8000000000`).

## 8. Yang masih gelap (alasan belum diport)

1. **Offset field di dalam state `iq`** tidak konsisten bila dibaca dari
   relokasi: sebagian akses memakai basis `.bss+0x08` (`&iq`), sebagian
   `.bss+0x10` (`iq+8`). Artinya ada anggota pertama berukuran 8 byte yang
   tidak pernah diset oleh probe (kandidat: `struct device_node *` atau
   pointer `struct resource`). Menulis driver berarti menebak layout itu, dan
   salah tebak = menulis ke memori reserved milik modem.
2. **Arti `mode` 0/1/2 dan magic `0x81005a00` / `0x81815a5a` /
   `0x4441504d` / `"PNOB"` / `"IPCD"` / `"KBLC"`** hanya terbaca sebagai
   konstanta; arah transisinya (siapa menulis apa lebih dulu) belum
   terverifikasi terhadap sisi modem/tool.
3. `_dev_info` "iq base/size" dan `mmap` memakai PFN dari VA — konversi ke
   6.18 harus diuji di perangkat mode IQ, tidak bisa diverifikasi di CI
   biasa.

Yang **tidak** lagi menjadi penghalang: `usb_f_vser` (fungsi gadget vendor)
sudah ada di `drivers/usb/gadget/function/f_vserial.c` dan mengekspor
`kernel_vser_register_callback`, `kernel_vser_set_pass_mode`,
`vser_pass_user_write` (commit sebelumnya), dan API SIPC `_ex` sudah
tersedia. Jadi sisa pekerjaan murni pada model state/mailbox di atas.
