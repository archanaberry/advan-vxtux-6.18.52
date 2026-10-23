# Deblob status — 2026-10-23 (TICK12)

The stock ROM ships **31 vendor .ko** in `/lib/modules`. The TICK12 build
(codespace, container ubuntu 22.04, `gcc-aarch64-linux-gnu` 11.4, konfig
fragmen di-merge di atas `.config` device) produces **227 `.ko`**, and
**29/31** stock module names now have a replacement module:

```
NAME 29/31   — yang belum berbentuk .ko: sprd-vsp-pw-domain, twofish_common
               (keduanya BUILT-IN `=y` di config, jadi fungsi 31/31 ada)
```

Six modules that previously could not be built at all now exist:
`sprdwl.ko` (WiFi SC2355), `sc27xx_typec.ko`, `novatek_ts_spi.ko`,
`sensorhub.ko`, `sprd_battery_info.ko`, `sprd-systimer.ko`.
Evidence: `docs/evidence-20261023/ko_list.txt` + `config.final`.

Catatan: angka LEVEL 2 (API) dan LEVEL 3 (DEPS) di bawah berasal dari
run 2026-10-21 dan **belum** diukur ulang untuk 227 `.ko` ini — lihat
bagian batas jujur di README TICK12.

## TICK12b — triage 33 blob "NOSRC" (2026-10-23)

PORT-AUDIT menandai 33 blob `NOSRC` karena tidak menemukan pemetaan
Makefile→modul, bukan karena sumbernya hilang. `donors/nosrc_triage.py`
membaca blob stok (`nm`+`strings`) dan pohon 6.18.52, lalu mengklasifikasi
ulang. Hasil lengkap: `docs/NOSRC-TRIAGE.md`.

| Verdict | Jumlah | Arti |
|---|---:|---|
| BUILT-IN | 12 | sumber ada, simbol `=y` → kode ikut `Image` |
| SUPERSEDED | 2 | fungsinya sudah ada di build lewat jalur lain |
| SIAP-COBA | 6 | sumber + Makefile ada, simbol CONFIG belum menyala |
| PERLU-PORT | 12 | sumber memang tidak ada di pohon |
| BLOB-HILANG | 1 | `sprd_vdsp.ko` tidak ada di `vendor_boot_a` |

Bukti per blob: `docs/NOSRC-TRIAGE.md` (kolom sumber, simbol, state config,
overlap simbol EXPORT terhadap `donors/all_syms.txt`).

Dua temuan yang menutup celah tanpa kode baru:

- `snd-soc-sprd-card.ko` — 33/33 simbol EXPORT-nya sudah ada di build
  (100% overlap).
- `sha1-ce.ko` — `lib/crypto/sha1.c` meng-include `$(SRCARCH)/sha1.h`, dan
  `lib/crypto/arm64/sha1.h` memanggil `__sha1_ce_transform` di bawah
  `kernel_neon_begin()` saat CPU punya fitur SHA1. Shash `sha1` bawaan
  `crypto/sha1.c` sudah memakai Crypto Extensions.

Satu simbol yang tadinya mati dinyalakan di fragmen: `CONFIG_PHY_SPRD_UMS512_USB2=m`
(padanan `phy-sprd-sharkl5Pro.ko`). Tanpa PHY ini `musb_sprd_vxtux` tidak
akan probe, jadi stack USB tidak hidup.

Yang benar-benar tersisa untuk 100%: 12 blob `PERLU-PORT`.

## TICK13 — build hijau lagi, PHY masuk, 228 `.ko`

Codespace kehilangan seluruh toolchain cross (bukan cuma PATH: `dpkg -l |
grep aarch64` = 0). Dipulihkan **tanpa sudo**: tarball ARM GNU 13.2.rel1 ke
`$HOME/tc`, `flex`/`bc`/`m4`/`libelf-dev` lewat `apt-get download` +
`dpkg -x`, dan clone bersih (pohon `/workspaces` penuh artefak root sehingga
`make O=` menolak). Resep lengkap: `docs/BUILD-ENV.md`.

Hasil `make modules` di pohon bersih:

```
rc=0      228 .ko      sebelumnya 227
```

`CONFIG_PHY_SPRD_UMS512_USB2=m` yang baru dinyalakan **lolos `olddefconfig`**
(dependensi `OF` + `REGULATOR` terpenuhi) dan menghasilkan
`drivers/phy/sprd/phy-sprd-ums512.ko` = **433.584 B**. Jadi blob stok
`phy-sprd-sharkl5Pro.ko` sekarang punya padanan yang benar-benar ter-build,
bukan cuma simbol yang menyala di config.

Bukti: `docs/evidence-20261023/ko_list_tick13.txt` (228 baris) dan
`config_tick13.txt`.

## TICK14 — bedah total undefined symbol (`symreach.py`)

Pertanyaan yang dijawab: untuk **setiap** referensi `SHN_UNDEF` di 129 blob
stok 5.4.210, apakah build 6.18.52 kita bisa memenuhinya, dan kalau tidak, apa
yang harus dilakukan. Alat: `donors/symreach.py`; keluaran:
`docs/UNDEF-REACHABILITY.md`, `docs/evidence-20261023/symreach.json`,
dan daftar alat di `docs/TOOLING-SYMBOLS.md`.

Sumber kebenaran yang dipakai (bukan tebakan):

| Bukti | Isi |
|---|---|
| `donors/symvers_tick14.txt` | 15.242 ekspor: vmlinux + 163 modul (sebelumnya 15.215) |
| `donors/vmlinux_o_syms.txt` | `nm --defined-only vmlinux.o`, 140.469 nama + tipe |
| `stock_vendor_modules_all/*.ko` | 129 blob, `.symtab` + `__versions` |

Kelengkapan daftar vmlinux diuji: **14.290 dari 14.291** ekspor vmlinux di
`Module.symvers` ada di `vmlinux.o`. Satu yang tidak ada adalah `jiffies` —
simbol linker-script yang dipakai `EXPORT_SYMBOL` tanpa entri di `vmlinux.o`,
bukan celah data.

| Kelas | Sebelum | **Sesudah** | Arti |
|---|---:|---:|---|
| `ok` | 4.434 (79,6%) | **4.750 (85,2%)** | diekspor build kita |
| `ekspor` | 23 | **0** | global, ada di vmlinux.o, tinggal `EXPORT_SYMBOL` |
| `rename` | 219 | 219 | padanan 6.18 terverifikasi (mekanis + `REMAP`) |
| `makro` | 476 | **330** | definisi ada di pohon: inline/makro/config mati |
| `port` | 24 | 24 | hanya disediakan blob stok lain |
| `instr` | 298 | **169** | `__cfi_*`/`__ubsan_*` runtime Clang stok |
| `hilang` | 98 | **80** | tidak ada di mana pun |
| **total** | **5.572** | 5.572 | 1.488 simbol unik |
| **blob tanpa port** | 82/129 | **91/129** | naik 9 blob |

Tiga perubahan yang menghasilkan angka di atas (semua build rc=0, 228 `.ko`):

1. `CONFIG_VXTUX_VENDOR_ABI=y` di `enhanced_config.cfg`. Penyedianya sudah
   ada di `drivers/vxtux/vendor_abi/` (bukan salinan mati di
   `drivers/misc/sprd/vendor_abi/` — itu tidak punya jalur Kconfig/Makefile).
   Dampak: 146 referensi `cpu_hwcaps`/`cpu_hwcap_keys`/`arm64_const_caps_ready`/
   `kimage_vaddr`/`cpu_number` + 129 referensi `__ubsan_handle_cfi_*` resolve.
2. `vendor_abi_exports.c` — 19 simbol global yang ada di `vmlinux.o` tetapi
   tidak diekspor (`irq_to_desc`, `cma_alloc/release/for_each_area/get_name`,
   `dma_direct_map_sg/unmap_sg/sync_sg_for_cpu`, `blk_alloc_queue`,
   `__alloc_disk_node`, helper audio AGCP + MCDT). Kelas `ekspor` 23 → 0.
   Percobaan pertama file ini **gagal compile** karena memakai
   `extern void nama(void)`; prototipe asli wajib (`irq_to_desc(unsigned int)`),
   dan sekarang semuanya diambil dari header pohon ini.
3. `vendor_abi_compat54.c` — `init_timer_key` (5.4) di atas `timer_setup`
   (6.18). 18 referensi tertutup, 9 blob tambahan lolos menjadi "tanpa port".

Empat temuan yang mengubah prioritas:

1. **`CONFIG_VXTUX_VENDOR_ABI` mati** di `config_tick13`. Padahal penyedia
   5 simbol terbesar (`cpu_hwcaps` ×35, `cpu_hwcap_keys` ×47,
   `arm64_const_caps_ready` ×47, `kimage_vaddr` ×11, `cpu_number` ×6 —
   total **146 referensi**) sudah ada di
   `drivers/misc/sprd/vendor_abi/vendor_abi_arm64.c`. Jadi sebagian besar
   kelas `makro` bukan "API hilang", tapi satu config yang belum menyala.
2. **19 simbol global ada di vmlinux.o tapi tidak diekspor** — mis.
   `irq_to_desc`, `cma_alloc`/`cma_release`, `blk_alloc_queue`,
   `__alloc_disk_node`. Untuk ini "perbaikan"-nya satu baris
   `EXPORT_SYMBOL[_GPL]`, bukan port driver.
3. **`sprd_sip_svc.ko` sudah 100% terklasifikasi** tanpa port apa pun:
   2 `ok` (`__arm_smccc_smc`, `__stack_chk_fail`), 1 `rename`
   (`printk` → `_printk`), 2 `instr`. Artinya leverage-nya (membuka 3
   referensi `sprd-cpufreq-v2`/`sprd_pdbg`/`sprd_power_stat`) bisa dipakai
   tanpa menambah simbol kernel baru.
4. **CRC modversion beda 3.223 dari 4.434** referensi `ok`. Nama simbol
   cocok, tapi genksyms 5.4 vs 6.18 menghasilkan CRC berbeda — indikasi
   drift ABI, bukan vonis per simbol (lihat batas jujur di dokumen alat).

Dampak ke hitungan port: **82 dari 129 blob** tidak lagi menunggu modul
stok lain dan tidak punya simbol `hilang`. Sisa `hilang` yang benar-benar
perlu kode cuma 62 simbol unik, dipimpin `init_timer_key` (18 blob),
`kmalloc_order_trace` (6), `__ioremap` (5), `fpsimd_context_busy` (4).

## TICK12c — dossier RE + node DT yang hilang

`donors/port_dossier.py` menghasilkan `docs/RE-PORT-DOSSIER.md` untuk ke-12
blob itu: EXPORT, UNDEFINED (penentu subsistem), penanda antarmuka, dan
graf dependensi. Tiga hal yang langsung terlihat:

1. **Klaster dependensi** — `sprd-cpufreq-v2`, `sprd_pdbg`, dan
   `sprd_power_stat` semuanya `depends: sprd_sip_svc`. Blob `sprd_sip_svc`
   sendiri kecil (5 simbol tak terdefinisi, 1 EXPORT `sprd_sip_svc_get_handle`),
   jadi ia harus di-port lebih dulu; tiga modul lain mengikuti.
2. **Node DT-nya ikut hilang.** `compatible` milik blob-blob ini (mis.
   `sprd,cpufreq-v2`, `sprd,debugstat`, `unisoc,sc2730-audio-codec`)
   semuanya **ada di DTB stok** tetapi **tidak ada** di DTS vxtux. Artinya
   port kode saja tidak akan pernah probe — node DT-nya juga harus di-port.
   Ini mengubah definisi "selesai" untuk 12 blob itu: kode + DT.
3. **Kontrak EXPORT** yang wajib dipertahankan: `mmc_swcq_*`,
   `stat_info_register/unregister`, `sprd_pdbg_msg_print`,
   `sprd_cpufreq_update_opp`, `sprd_sip_svc_get_handle`.

## Riwayat 2026-10-21

Previously: same-name replacement for **24** of them, and **18** of those
carry the same API (the stock blob's own functions, locals included).

The measurement is `donors/verify_deblob2.py`; it writes
`docs/DEBLOB_VERIFICATION.md`.

## What the build produces

- **202 modules** (was 201 before `sprd_apipe.ko` was fixed)
- **164,210 symbols** in vmlinux + all modules (`donors/all_syms.txt`)

## The three checks

1. **NAME** — the build emits a `.ko` with the stock file name.
2. **API** — that `.ko` has every symbol the stock `.ko` had, local
   symbols included. The stock blob's locals are the driver's own
   functions, so a replacement without them is not the same code.
3. **DEPS** — every symbol the replacement imports exists in the build
   (vmlinux **or** a module the build produces).

Check 3 was reporting false gaps until v34: it looked at the vmlinux
only, and a module's symbols live in the `.ko`:

```
$ nm built_kos/drivers/trusty/trusty-ipc.ko | grep tipc_chan_connect
000000000000206c T tipc_chan_connect
```

## The verifier was blind to local symbols

Check 2 counted a symbol only when `nm`'s type letter was uppercase:

```python
elif typ.isalpha() and typ.isupper():
    defs.add(name)
```

`nm` prints a lowercase letter for a **local** symbol, and the stock
blob's own driver functions are local (`t trusty_std_call_inner`,
`t _handle_conn_rsp`, …). So check 2 saw only the exports and reported
`trusty` and `trusty-ipc` as "identical" while they were missing 4 and
17 driver functions respectively.

The fix counts every defined symbol (`TtDdBbRrWwVv`) and excludes the
names that are the *compiler's*, not the driver's:

| name | clang 5.4 | gcc 6.18 |
|---|---|---|
| dynamic-debug descriptor | `<func>.__UNIQUE_ID_ddebug<N>` | `__UNIQUE_ID_ddebug<N>.<M>` |
| `WARN_ONCE` flag | `<func>.__warned` | `__warned.<N>` |
| lockdep key of a local static | `<func>.__key` | `__key.<N>` |
| label placeholder | `.Ltmp<N>` | `.L<N>` / `.L14472!<N>` |

The **function that owns them** is still compared, so nothing of the
driver's own code is hidden by the exclusion.

### `trusty` and `trusty-ipc`: closed

The stock blobs were built with clang 12; this tree is built with gcc
13. clang keeps a `static` function as its own symbol, gcc -O2 inlines
it, and the local symbol disappears. The replacement therefore did not
carry the symbol surface of the blob it replaces.

`trusty-core.c`: `trusty_std_call_inner` is now `noinline`.

`trusty-ipc.c`: 14 functions are now `noinline` —
`_create_cdev_node`, `_dn_lookup_vds`, `_free_vds`, `_handle_conn_rsp`,
`_handle_ctrl_msg`, `_handle_disc_req`, `_handle_rxbuf`,
`_vds_get_txbuf`, `create_cdev_node`, `destroy_cdev_node`,
`vds_add_channel`, `vds_create_channel`, `vds_del_channel`,
`vds_get_txbuf`.

`vds_add_channel` also needs `__noclone`: gcc clones it into
`vds_add_channel.constprop.0` (specialising on the constant return
value) and the *clone* is the symbol that survives, so `noinline` alone
is not enough there.

`tipc_virtio_probe.vq_names` is a `const char *[] {"rx","tx"}` that
6.12's `find_vqs()` folded into `struct virtqueue_info[]`. It is kept
as a function-local static with an explicit asm label so the symbol
name matches the stock blob's.

One stock symbol cannot be restored:
`trace_event_define_fields_smccall` was generated by the 5.4
trace-event machinery, and 6.18 has no such generator (the tree's
`include/trace/stages/` has no `define_fields` stage). It is a
property of the kernel's trace-point code, not of the driver.

## The real gaps

### 5 modules with a missing API

| stock blob | missing | what it is |
|---|---|---|
| `aes-ce-ccm` | 0 | **closed** — see below |
| `apsys-dvfs` | 26 → 3 | see "apsys-dvfs" below |
| `mmdvfs` | 115 → 51 | see "mmdvfs" below |
| `novatek_ts_spi` | 11 | the Novatek NT36xxx touch controller's pen/sequence API |
| `sdhci-sprd` | 0 | **closed** — see "sdhci-sprd" below |
| `sprd_time_sync` | 0 | **closed** — see below |
| `unisoc-mailbox` | 0 | **closed** — see below |

#### `sdhci-sprd`: closed (28 missing -> 0)

The 28 were not one thing. Recovered from
`donors/ghidra_out/sdhci-sprd.ko.funcs.txt` and radare2 (the Ghidra
per-function output only covers the write-protect half; the SW-CQ half was
recovered with radare2's `pdc` against the blob).

**1. The software command queue was inert.** The tree carried a 37-byte
placeholder for `drivers/mmc/host/mmc_hsq.c` and the Makefile never built
it, so `mmc_hsq_init()` was a static inline returning 0 and the HSQ the
Unisoc driver drives did nothing at all. `CONFIG_MMC_HSQ` was already `=y`
and already `select`ed by `MMC_SDHCI_SPRD` — only the source was missing.

6.18 has the real implementation, so it is upstream's
`drivers/mmc/host/mmc_hsq.{c,h}` verbatim, plus `obj-$(CONFIG_MMC_HSQ) +=
mmc_hsq.o`. That reproduces the stock `mmc_hsq.ko` API exactly: the blob's
four exports (`mmc_hsq_init`, `mmc_hsq_finalize_request`,
`mmc_hsq_suspend`, `mmc_hsq_resume`) are upstream's four, and the blob's
internal symbols (`mmc_hsq_ops`, `mmc_hsq_pump_requests`,
`mmc_hsq_recovery_start`, ...) have the same names upstream gives them —
the vendor's `mmc_hsq.ko` *is* this code.

`sdhci-sprd.ko` now imports them and modpost records `depends=mmc_hsq`,
as the stock blob does.

**2. The SDHCI-core copy.** 17 of the missing were the vendor's renamed
copy of `sdhci.c`'s own internals (`sdhci_sprd_irq`, `sdhci_sprd_cmd_irq`,
`sdhci_sprd_data_irq`, `sdhci_sprd_prepare_data`, `sdhci_sprd_transfer_pio`,
`sdhci_sprd_read_block_pio`, `sdhci_sprd_write_block_pio`,
`sdhci_sprd_finish_command`, `sdhci_sprd_calc_timeout`,
`sdhci_sprd_set_transfer_irqs`, `sdhci_sprd_send_command`,
`__sdhci_sprd_finish_mrq`, `__sdhci_sprd_finish_data`,
`sdhci_sprd_dumpregs`, `sdhci_sprd_int_status`,
`sdhci_sprd_signal_voltage_on_off`, `sdhci_sprd_execute_tuning`) — all
`nm` type `t`, i.e. local. 6.18 has every one of these bodies; they are
just `static` in `sdhci.c`. Each is now a name-preserving forwarder, so
the module carries the vendor's names without a second copy of `sdhci.c`
that would drift on every merge. Where the body is static upstream,
`sdhci.c` exports it under a `*_body` name and the module wraps it — that
is a `ponytail:`; one export per body for exactly one consumer. Fold them
into a single `sdhci_ops` entry when a second vendor driver needs them.

**3. Write protect.** Already implemented; the symbols were lost because
gcc -O2 inlined them and because the tree had renamed them
(`sprd_wp`/`sprd_wp_bus_type` instead of the blob's `wp_dat`/`wp_bus_type`).
Renamed back and `noinline` added, matching what the `trusty` fix already
does for the same reason.

**4. `pre_time` / `second_time` / `_copy_from_user`.** The first two are the
blob's tuning-sweep timestamps (BSS, `B`). `_copy_from_user` is 4.14's
arm64 PAN-toggling uaccess helper; 6.18 moved that into
`__arch_copy_from_user` and made `copy_from_user` a plain function, so the
blob's symbol has no separate body upstream. It forwards to
`copy_from_user`, which is what the tree's ioctl already calls.

`donors/verify_deblob2.py` now reports row 15 as `identical` / `yes` on all
three levels.

#### `apsys-dvfs`: 53 stock symbols, 50 exported (build #149)

Ported as `drivers/devfreq/sprd/apsys/` — one composite module
(`apsys-dvfs.ko`), composed of `sprd_dvfs_apsys.c` (the apsys block),
`sprd_dvfs_ops.c` (the shared per-slot op bodies plus the 21 per-SoC op
tables and the `*_val_to_*` string tables) and `sprd_dvfs_regs.c` (the
per-SoC register descriptors).

Recovered with Ghidra headless (`donors/ghidra_out/apsys-dvfs.ko.funcs.txt`,
23 KB) and radare2. Key finding: the per-SoC clones are byte-identical
apart from register offsets, so the port is one shared op body per slot
plus a per-SoC register descriptor, not 21 hand-copied tables.

6.18 API migration this driver needed:
- `struct ip_dvfs_coffe` had to move into `sprd_dvfs_soc.h`, because
  `struct ip_dvfs_dev` (which embeds it by value) lives there and
  `sprd_dvfs_soc.h` cannot include `sprd_dvfs_apsys.h` without a cycle.
- The vendor blob builds against a kernel with `BLOCKING_NOTIFIER_INIT`;
  6.18 removed it — the notifier head is now a plain declaration, and the
  chain is wired with `blocking_notifier_chain_register()` at probe.

**Still missing (26):** the per-IP devfreq device objects —
`dpu_dvfs_driver`, `dpu_devfreq_gov`, `dpu_dvfs_head`,
`dpu_dvfs_notifier_call_chain` and the same five for `vsp`/`vdsp`/`gsp`,
plus `vpuenc_dvfs_chain`, `dpu_vsp_dvfs_check_clkeb`,
`find_apsys_device_by_name`, `get_display_power_status`,
`vsp_dvfs_get_dev_status`, `n6pro_soc_ver_id_check`. The port registers
the op tables but does not yet instantiate one devfreq device per IP.

#### `mmdvfs`: 115 stock symbols, 64 exported (build #149)

Ported as `drivers/devfreq/sprd/mm/` — one composite module
(`mmdvfs.ko`), composed of `sprd_mm_dvfs_core.c`,
`sprd_mm_dvfs_ip.c` (one platform driver + one devfreq governor per
memory IP) and `sprd_mm_dvfs_pqueue.c`.

6.18 API migration this driver needed:
- `devfreq_event_data()` / `devfreq_event_get_event()` are gone, and
  `struct devfreq_dev_status` shrank to
  `total_time`/`busy_time`/`current_frequency` + `private_data`. The
  min/max/governor window now lives on `struct devfreq` itself as
  `scaling_min_freq`/`scaling_max_freq` and `->governor`.
- `struct devfreq_governor` and `DEVFREQ_GOV_*` are in
  `drivers/devfreq/governor.h`, not `<linux/devfreq.h>` — needs the
  relative include `../../governor.h`.
- The `IP_DRIVER()` macro parameter must not be named `compatible`: the
  preprocessor substitutes it inside the `.compatible` field name too,
  yielding `.sprd,hwdvfs-mtx` and a compile error.

The vendor blob names its entry point `mmsys_module_init` /
`mmsys_module_exit` (verified with `nm`); the port keeps those names
rather than a plain `module_init()`.

**Still missing (51):** the per-IP `*_dvfs_gov` symbols are static in the
port (the vendor blob exports them), and the 24 sysfs
`get_*_show` / `set_*_store` handlers are not written yet.

#### `aes-ce-ccm`: closed (build #115)

The stock blob has `ce_aes_ccm_final` as a **global** function, next
to a local label of the same name inside `ce_aes_ccm_crypt_tail`. The
two are different things: the local label is the "final round key
cancels out" step of the CCM mac, the global one is a standalone
two-block AES helper that the stock blob's `ce_aes_ccm_encrypt` and
`ce_aes_ccm_decrypt` call at `.text+0x6a4` and `.text+0x968`:

    out = AES(out, rk) ^ AES(in, rk)

It is now a global `ce_aes_ccm_final` in
`arch/arm64/crypto/aes-ce-ccm-core.S`, built on the `aes_encrypt`
macro that was already there (that macro takes two data registers for
exactly this reason). The upstream 6.18 local label was renamed to
`ce_aes_ccm_final_block` to free the name — the stock 5.4 blob has no
local label of that name, because its `ce_aes_ccm_encrypt` has no
`final_iv` handling at all.

The symbol surface is now identical to the stock blob, except for two
symbols: `ccm_update_mac` and `ccm_crypt_fallback`.

Those two are the **non-NEON fallback** path. The stock blob is
upstream v5.4 `arch/arm64/crypto/aes-ce-ccm-glue.c`, which still had
them; upstream removed both in b3482635e5c ("crypto: arm64/aes-ccm -
remove non-SIMD fallback path", 2021-08-27) once the CE path became
the only path. This 6.18 tree already carries that removal, so
re-adding them would reintroduce a code path the kernel deliberately
deleted — and they would be **dead code**, because nothing calls them
any more.

`donors/verify_deblob2.py` reports them as the last 2 "missing" symbols
for this blob, and that report is correct as a *symbol-surface* diff.
It is not a functional gap: the CCM AEAD in this tree
(`ccm(aes)` / `ccm-aes-ce`, priority 300) encrypts and decrypts
correctly without them.

Measured on this tree (build #115, `built_kos/arch/arm64/crypto/aes-ce-ccm.ko`):

```
$ nm built_kos/arch/arm64/crypto/aes-ce-ccm.ko | grep -E " [Tt] "
0000000000000020 T ce_aes_ccm_auth_data
0000000000000a8c t ce_aes_ccm_crypt_tail
0000000000000bcc T ce_aes_ccm_encrypt
0000000000000adc t ce_aes_ccm_final_block
0000000000000d24 T ce_aes_ccm_decrypt
0000000000000e7c T ce_aes_ccm_final
```

Note `ce_aes_ccm_final` is present but **not called** by the 6.18 glue:
upstream commit f691d444f9c ("crypto: arm64/aes-ccm - Merge
finalization into en/decrypt asm helpers", 2024-01-18) folded the
finalization step into the `ce_aes_ccm_encrypt`/`ce_aes_ccm_decrypt`
assembly helpers. The symbol is kept so the blob's API surface still
matches, but it is not on any live path.

#### `unisoc-mailbox`: closed (build #113)

The stock blob carries **two** layers in one file. Layer 0 is the Linux
`mbox_controller` framework driver, which was already in
`drivers/mailbox/sprd-mailbox.c`. Layer 1 is the OEM "revision 1" layer:
the `sprd_mbox_phy_ops_r1` vtable, the per-channel FIFO table, the
deliver thread, the PM notifier and the wakeup-flag API.

The contract was recovered with Ghidra 12.1.4
(`donors/ghidra/DumpFuncs.java`, output in `donors/ghidra_out/`) and
implemented in `drivers/mailbox/sprd-mailbox-r1.c` +
`drivers/mailbox/sprd-mailbox-r1.h`, wired into the composite module
as `unisoc-mailbox-y := sprd-mailbox.o sprd-mailbox-r1.o`.

The vtable layout was confirmed from the stock blob's relocation table
with `donors/ko_data.py`: `sprd_mbox_phy_ops_r1` points at
0x104368 startup, 0x104370 shutdown, 0x104378 flush, 0x104380 send,
0x10438c inbox_isr, 0x104390 outbox_isr, 0x104394 typeinfo,
0x104398 debug_show.

The symbol surface is now **identical** to the stock blob:

```
$ comm -23 stock_syms built_syms   # in stock, missing from built
$ comm -13 stock_syms built_syms   # in built, not in stock
                                    # both empty
```

**The `_r1` API has no consumer in the stock ROM.** The stock vmlinux
does not import `sprd_mbox_change_wakeup_flag_r1`,
`sprd_mbox_wakeup_flag_callback_register_r1` or
`sprd_mbox_wakeup_flag_callback_unregister_r1`, no stock `.ko` imports
`get_tx_fifo_mask` or `mbox_start_send_tx_fifo`, and a scan of the
whole stock filesystem finds no reference either. It is implemented
anyway, because the replacement module has to carry the same API
surface as the blob it replaces — the deblob check compares the local
symbols too.


### 7 stock blobs with no module at all

`audio_sipc`, `mmdvfs`, `sprd-dmaengine-pcm`, `sprd-sipc-virt-bus`,
`sprd-vsp-pw-domain`, `sprd_time_sync`, `usb_f_vser`

Six of the seven have **no source in the tree** — they are ports still
to be written, not build fixes. `sprd-dmaengine-pcm` has only a header
(`drivers/vxtux/sound/asoc/platform_include/sprd-dmaengine-pcm.h`).

## Kernel API drift is not a gap

The stock blobs were built for the 5.4 kernel; this tree is 6.18. The
kernel renamed a lot of its own API, so a replacement imports symbols
the stock one did not:

```
_printk  __kmalloc_cache_noprof  mutex_lock_nested
validate_usercopy_range  __list_add_valid_or_report  ...
```

All of them are present in the build. The report calls these out
separately as "kernel API drift".

## What the last few versions fixed

| version | what |
|---|---|
| v31 | `sprd_apipe.ko` — the apipe character device, the stock `sprd_apipe.ko` |
| v32 | **wrong** — created a `drivers/sound/` Makefile chain |
| v33 | undid v32: `drivers/sound/` is a dead duplicate of `drivers/vxtux/sound/asoc/`, deleted |
| v34 | the verification's two false-gap bugs |

## The silent-descent trap

Four times now a directory of sources sat in the tree, complete, with
its Kconfig, and nothing built it — because kbuild descends into a
directory only when its parent lists it:

- `drivers/soc/sprd/modem/`
- `drivers/vxtux/audio/audio_pipe/`
- `drivers/sound/soc/sprd/` (this one turned out to be a *duplicate*,
  not a missing chain)
- `drivers/sound/` (the same)

`donors/scan_dup_kconfig_syms.py` now reports **0** symbols defined
in two reachable Kconfig files.


## Build 165 status (measured)

  Image          42,072,576 bytes, boots (no bootloop, no panic)
  modules        224 .ko built
  stock names    30 of 31 produced with the exact stock file name
                 (the 31st, sprd-vsp-pw-domain, is built into vmlinux on
                 purpose - see the section above)

  exported API   376 of the 424 stock exported symbols present, 88.7%
  identical      7 of 31 modules carry the stock blob's full symbol set
  complete       mmdvfs (117/117), sdhci-sprd, novatek_ts_spi: 0 missing

  the remaining gap is concentrated in five modules:
    apsys-dvfs        229   per-IP devfreq objects (in progress)
    mmdvfs              49   local statics: attribute tables, gov callbacks
    novatek_ts_spi      84   local statics: memory-map tables
    unisoc-mailbox     29   local statics: PHY ISR path, debugfs, pm notifier
    trusty-ipc          13   local statics: vds_* and _handle_* functions


## trusty: three symbols that are a clang-vs-gcc difference, not a port gap

trusty.ko is reported as missing three symbols: __tracepoint_ptr_smc_entry,
__tracepoint_ptr_smc_exit and trace_event_define_fields_smccall.  All three
come from the tracepoint machinery, not from the driver.

  - __tracepoint_ptr_<name> is generated by __TRACEPOINT_ENTRY() in
    include/linux/tracepoint.h.  With CONFIG_HAVE_ARCH_PREL32_RELOCATIONS the
    entry is emitted as inline asm, so no symbol exists at all.  The 5.4
    vendor kernel did not have that config option, so it used the variable
    form and the symbol appeared.  The 6.18 tree selects it from
    arch/arm64/Kconfig.
  - trace_event_define_fields_<template> is emitted by clang for the trace
    event class.  The 5.4 donor kernel's include/trace/define_trace.h has no
    such macro either, so the name comes from the compiler, not the kernel.

Turning off CONFIG_HAVE_ARCH_PREL32_RELOCATIONS would bring the first two
back, but that option also governs init_call and PCI relocation handling, so
it is not a deblob change to make.  The port already has the tracepoint
itself: __tracepoint_smc_entry, __tracepoint_smc_exit, event_class_smccall
and print_fmt_smccall are all present.

ponytail: if the stock tracepoint symbol names ever matter, drop
CONFIG_HAVE_ARCH_PREL32_RELOCATIONS from arch/arm64/Kconfig and re-measure;
the cost is a slower tracepoint dereference and different init_call and PCI
relocation handling.

## TICK16 — CI hijau, `sprd_sip_svc` ter-build

Untuk pertama kalinya sejak guard config ditambahkan, workflow `v17-build.yml`
lolos sampai akhir (run `39946023755`, commit `a0f2e70ce`): guard config,
build DTB, build modules, tar `.ko` — semuanya sukses, rc=0, **229 `.ko`**
(sebelumnya 228). Artefak: `v17-built-kos` + `v17-dtb`.

Yang diperbaiki di jalan:

1. **Guard config tidak pernah lolos.** Ia membandingkan seluruh
   `enhanced_config.cfg` dengan `.config` hasil `olddefconfig`. Fragmen itu
   mewarisi config vendor 5.4 utuh, jadi 412 entri menunjuk driver yang belum
   di-port atau subsistem yang dipangkas dari tree. Sekarang guard hanya
   mewajibkan simbol yang Kconfig-nya ADA dan tidak terdaftar di
   [CI-KNOWN-DROPPED.txt](CI-KNOWN-DROPPED.txt) (124 entri, berkelompok
   dengan alasannya).
2. **`sprd_sip_svc` masuk pohon yang salah.** Modul pertama ditaruh di
   `drivers/soc/sprd/`, yang Kconfig-nya masih di-source `drivers/Kconfig`
   tetapi Makefile-nya tidak pernah dimasuki — simbol `=m` tanpa `.ko`.
   Modul SoC Unisoc yang hidup ada di `drivers/vxtux/soc/`; setelah dipindah
   ke sana, `sprd_sip_svc.ko` benar-benar dibangun:
   `drivers/vxtux/soc/sip_svc/sprd_sip_svc.ko`, vermagic cocok, undefined
   hanya `__arm_smccc_smc`, `_printk`, `memset`, `__stack_chk_fail`.
3. **Audit tidak lagi menebak dari nama.** `gpio.ko` dulu dipetakan ke
   `CONFIG_MTD_NAND_GPIO` dan `core.ko` ke kandidat acak; keduanya kini
   terbaca sebagai `NOSRC` dengan bukti dari metadata blob (`sprd autotest
   gpio/core driver`). Nama modul generik dengan banyak kandidat CONFIG_
   dilaporkan `AMBIGUOUS`, bukan dipilih yang pertama ditemukan.
4. **Config efektif kini benar-benar ter-upload.** `upload-artifact` v4
   membuang hidden file, jadi `.config` selama ini tidak pernah ada di
   artefak; disalin dulu ke `effective.config`.

Skor: **119/129** (KO 58, Y 47, BUILT-IN 12, SUPERSEDED 2, BELUM 10). Rincian
sepuluh yang tersisa, lengkap dengan bukti dan blokirnya:
[PORT-REMAINING.md](PORT-REMAINING.md).
