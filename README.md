# VXTux 💜✨

<div align="center">

### Linux 6.18.52 for Advan Tab VX Lite (T1030) desu~ (≧▽≦)/

`ARM64` / `Unisoc UMS512-T618` / `VXTux`

**一歩ずつ、確実に。 Device-first kernel work, documented with care. (^_^)/**

*Halo nyaa~ Hello nyaa~ こんにちはなの~ 💕*

</div>

<div align="center">
  <img src="archanaberry/vxtux_chan.png" width="70%">
</div>

---

## 🌸 Tentang proyek | About | プロジェクトについてなの

> **ID:** Halo halo~ (｡•́︿•̀｡) VXTux ini kernel Linux yang fokus buat **Advan Tab VX Lite T1030** nyaa~ Targetnya Unisoc UMS512 / T618 desu! Gabungin driver upstream yang baik hati + port platform hati-hati + device-tree support~ Bukan config dump loh! (｀・ω・´) ✨

> **EN:** Hewwooo~ (≧∇≦)/ VXTux is a device-focused Linux kernel for our beloved **Advan Tab VX Lite T1030** nano~ Target is Unisoc UMS512 / T618 desu! Combines kind upstream drivers + carefully scoped ports + DT support nyaa~ (｡>﹏<｡)

> **JP:** こんにちはなの〜 (⁄˶´-`⁄)♡ VXTuxは大好きな **Advan Tab VX Lite T1030** のためのカーネルなの！ターゲットはUMS512 / T618なのです〜。upstreamの優しいドライバーと丁寧なポート、DTサポートを組み合わせてるの！(๑•̀ㅂ•́)و✧

---

## 🔗 Referensi donor | Donor references | ドナー参照だよっ

Tabel donor/referensi untuk keperluan reverse engineering (RE) desu~ Semua repo di bawah SoC-nya Unisoc UMS512 / T618 (sharkl5pro). **Angka = file vendor byte-identical di tree** (hash, CRLF-normalized; file yang diadaptasi tidak dihitung verbatim). Diurut dari terbanyak kepakai~

| Nama Repo | Sumber Repo (link) | Keterangan / Status |
| --- | --- | --- |
| cdpkp/android_kernel_tree_samsung_t618 | [github.com/cdpkp/android_kernel_tree_samsung_t618](https://github.com/cdpkp/android_kernel_tree_samsung_t618) | Samsung Tab A8 — **SoC T618 sama**. 67 file vendor verbatim di tree (terbanyak). Copy lokal: `gpl-source/fork_t618/extract`. Sesi ini: sumber struktur node DTS sharkl5pro (pola `syscons`/`syscon-names`) + `sprd_dvfs_vsp.h`. **(67) bagian kepakai** |
| iscle/android_kernel_unisoc_ums512 | [github.com/iscle/android_kernel_unisoc_ums512](https://github.com/iscle/android_kernel_unisoc_ums512) | Donor utama sesi ini: VSP (4 file), ion ipc, dvfs header. 55 file vendor verbatim di tree. Copy lokal: `work/donors_20260930/iscle_ums512/`. **(55) bagian kepakai** |
| Bocchi-The-Dev/kernel_unisoc-5.4 | [github.com/Bocchi-The-Dev/kernel_unisoc-5.4](https://github.com/Bocchi-The-Dev/kernel_unisoc-5.4) | Fork realme C53 (MocorDroid 5.4). 58 file vendor verbatim. Copy lokal: `work/donors_20260930/bocchi_unisoc54`. **(58) bagian kepakai** |
| marohinmark/kernel_ums512_5.4 | [github.com/marohinmark/kernel_ums512_5.4](https://github.com/marohinmark/kernel_ums512_5.4) | Fork kernel ums512 5.4. 58 file vendor verbatim. Copy lokal: `work/donors_20260930/marohinmark_ums512`. **(58) bagian kepakai** |
| Hadenix/android_kernel_realme_ums512 | [github.com/Hadenix/android_kernel_realme_ums512](https://github.com/Hadenix/android_kernel_realme_ums512) | Referensi WCN/gnss sc2355. 21 verbatim; 80/81 nama-file ada di tree (diadaptasi). Copy lokal: `gpl-source/hadenix_ums512_wcn`. **(21) bagian kepakai** |
| sprd-oss-devs/android_kernel_realme_ums512 | [github.com/sprd-oss-devs/android_kernel_realme_ums512](https://github.com/sprd-oss-devs/android_kernel_realme_ums512) | Sumber BT tty-sdio + FM fm_sdio. 12 verbatim (22/22 nama-file ada di tree, diadaptasi). Copy lokal: `gpl-source/ums512_bt_fm`. **(12) bagian kepakai** |
| HimaxSoftware/HX83112_Android_Driver | [github.com/HimaxSoftware/HX83112_Android_Driver](https://github.com/HimaxSoftware/HX83112_Android_Driver) | Framework touch hxcommon. 5 verbatim (13/13 nama-file ada di tree, diadaptasi). Copy lokal: `gpl-source/upstream/himax_hx83102_sprd`. **(5) bagian kepakai** |
| TWRP Device Tree ADVAN_TAB_VX_LITE | [twrpdtgen/android_device_advan_ADVAN_TAB_VX_LITE](https://github.com/twrpdtgen/android_device_advan_ADVAN_TAB_VX_LITE/tree/ums512_1h10_Natv-user-13-TP1A.220624.014-82490-release-keys) | Donor ramdisk hybrid TWRP: recovery binary + /sbin (libs, linker64, toybox, bash) + twres + prop.default. **(4) bagian kepakai** |
| beebono/rg-rotate-linux | [github.com/beebono/rg-rotate-linux](https://github.com/beebono/rg-rotate-linux) | Referensi bringup mainline UMS512/T618. 3 artefak dikutip docs kita: `DEVICE-BRINGUP.md`, `decomp_stock.dts`, `stock_live_booted.dts`. Clone lokal = docs+tools saja (kernel tidak). **(3) bagian kepakai** |
| MotorolaMobilityLLC/kernel-sprd | [github.com/MotorolaMobilityLLC/kernel-sprd](https://github.com/MotorolaMobilityLLC/kernel-sprd) | `himax_ic_HX83102.c/.h` (touch HX83102) ada di tree. **(2) bagian kepakai** |
| realme-kernel-opensource/realme_C31_C33_C35_narzo50A-Prime | [realme-kernel-opensource/realme_C31_C33_C35_narzo50A-Prime-AndroidT-kernel-source](https://github.com/realme-kernel-opensource/realme_C31_C33_C35_narzo50A-Prime-AndroidT-kernel-source) | Fork MocorDroid-S kanonik — induk keluarga iscle/bocchi/marohinmark (angka di atas overlap: file yang sama bisa match beberapa fork). Belum di-clone. **(0 lokal)** |
| orangepi-xunlong/linux-orangepi | [github.com/orangepi-xunlong/linux-orangepi](https://github.com/orangepi-xunlong/linux-orangepi) | uwe5622 unisocwifi — referensi WLAN lintas-platform (generasi lama, bukan drop-in `wlan_combo`). Belum di-clone. **(0 lokal)** |
| Seriousattempts/rp3plus-native-attempts | [github.com/Seriousattempts/rp3plus-native-attempts](https://github.com/Seriousattempts/rp3plus-native-attempts) | Belum dipakai — nol copy lokal, nol kutipan di docs. **(0) bagian kepakai** |

> 💡 Catatan RE: donor terbesar = **T618 Samsung (67)** + keluarga fork MocorDroid 5.4 (iscle 55 / bocchi 58 / marohinmark 58 — overlap, fork dari base sama). Sesi ini: VSP+CPP diporting dari iscle/mods-sprd, node DTS vsp+cpp dari struktur sharkl5Pro-fork (keluarga T618). Motorola + Himax untuk touch, Hadenix/sprd-oss-devs untuk WCN/BT/FM.

---

## 🎯 Target | ターゲットだよっ

| Area | Target desu~ |
| --- | --- |
| Device | Advan Tab VX Lite T1030 📱 |
| SoC | Unisoc UMS512 / T618 |
| Architecture | ARM64 |
| Kernel | Linux 6.18.52-VXTux-UMS512 |
| Device tree | `ums512-1h10-vxtux.dtb` 🌟 |
| Graphics | Mali-G52 MP2 (Bifrost, Gondul) via Panfrost desu! |

---

## 💜 Driver tally | ドライバー集計なのっ

| Count | Scope nyaa~ |
| ---: | --- |
| 102 | Deblob-derived driver integrations verified in tree + config — **13 di antaranya compile-verified di device** (gcc 16.2.1, artix proot): vsp, cpp, iommu, ion, camsys/vsp pw-domain, battery_info, charger_manager, wcn_core, wlan_combo (`sprdwl.ko` 2,5 MB), jpg (`sprd_jpg.ko` 156 KB), **`sprd_vdsp.ko` 913.568 byte** (TICK7), **`sprd-gsp.ko` 767.488 byte** (TICK9), **`novatek_ts_spi.ko` 512.584 byte** (TICK9) |
| 1 | `sprd_flash_drv` + `spi-sprd` — closed via System.map (14 + 23 simbol build-in, TICK9, 14 Okt 22:18 WIB) |
| 1 | Panfrost open-source GPU driver replacing vendor Mali Gondul DDK |
| 41 | **Vendor feasible match** (41 dari 129 vendor .ko Advan T1030 yang match dengan 103 port — breakdown feasible: Audio 11, Power/Regulator 11, GPIO/Pinctrl/Clock 8, I2C/SPI/SDHCI 6, SIPC/Modem 12, Storage/MMC 4, Efuse/NVMEM 3, Sensor/Thermal 5, CPUFreq 3, USB Gadget 3, Trusty TEE 6, Graphics 2, WiFi/BT 2, Touch 1, Misc 6, Mainline ~30. **88 vendor-only** belum di-port → TICK11+). Lihat [`docs/DRIVERS_VENDOR_CROSSCHECK.md`](docs/DRIVERS_VENDOR_CROSSCHECK.md) + [`docs/MASTER.md`](docs/MASTER.md) |
| **144** | **Total tracked desu!** = 102 + 1 + 41 = **144 entry feasible** (102 port deblob + 1 GPU + 41 vendor match). **0 MISS (≧▽≦)/** |

> **Status TICK11 (2026-10-22) — thermal DT chain 3/4 SELESAI ditulis (RE 1:1 dari DTB stok):**
> - **Commit `efa7348c2`** (branch `test1`): `ap_efuse@32240000` (`sprd,ums312-efuse`/`sprd,ums512-efuse`, hwlocks `<&hwlock 8>`, clocks `CLK_EFUSE_EB`, konsumen `nvmem-sprd-efuse` + `CONFIG_NVMEM_SPRD_EFUSE=m`); `ap_thm0/1/2` = thermal@32200000/32210000/32220000 (`sprd,ums512-thermal`, `CLK_THM0/1/2_EB`, 12 node sensor `sen_delta_cal`); **24 sel cache eFuse** di `efuse@800` (thm0/1/2 sign+ratio+sen, gpu-bin, cpu-flag, dvfs-bin@17/@1b — reg/bits persis stok); `virtual-sensor@0` (`virtual-thermal`, `CONFIG_SPRD_SOC_THM=m`, `thmzone-cells` 10 zona urutan persis stok); **13 zona thermal baru** (`soc-thmzone` trips 70/85/110°C + cooling-map map0-4 → CPU0/CPU6/cluster0/1/gpu-cooling0 angka persis stok; prometheus6-tzone0/1, prometheus7, ank0-5, gpu, cputop, gpuank2); cooling-devices `cluster0/1-cooling` + `gpu-cooling0` (properti leak/scale/dynamic persis RE stok); `#cooling-cells = <2>` di CPU0/CPU6 (pemimpin cluster, persis stok); 8 sel kalibrasi PMIC `efuse@300` tersisa (total 13 persis stok). RE via `donors/fdt_dump.py` (pengganti dtc, mesin ini tanpa dtc) — bukti: `donors/RE_EFUSE_stock.dtb.txt`, `donors/RE_THERMAL_stock.dtb.txt`, `donors/RE_THERMALZONES_stock.dtb.txt`. CI `v17-build.yml` ditambah step build DTB + artifact `v17-dtb`.
> - **Batas jujur TICK11**: verifikasi = compile DTB + modul di GitHub Actions (bukan boot di hardware); `sprd,cluster-cooling` belum punya driver bind penuh (node disiapkan + `SPRD_CPU_COOLING=m` mem-parsa properti power model by-name); probe `sc27xx_tsensor` tetap butuh `androidboot.mode=cali` di bootargs (perilaku blob vendor).
>
> **Status TICK12 (2026-10-23) — build device nyata (codespace, bukan GitHub Actions), 5 driver vendor baru benar-benar ter-build, 29/31 .ko stok tercakup:**
> - **Build di Codespace** `afrizalsyahrezatzy101-hash/advan-vxtux-6.18.52` (branch `test1`, container ubuntu 22.04 + `gcc-aarch64-linux-gnu` 11.4): `merge_config.sh` fragmen **di atas `.config` device** (bukan menggantinya) → `olddefconfig` → `make modules -j4` = **rc=0, 227 `.ko`**. DTB `ums512-1h10-vxtux.dtb` juga ter-build dari config device (53.396 byte) → chain thermal TICK11 tetap hijau.
> - **5 driver vendor yang sebelumnya mustahil ter-build, sekarang ada `.ko`-nya**: `sprdwl.ko` (WiFi SC2355, 16,0 MB), `sc27xx_typec.ko` (366 KB), `novatek_ts_spi.ko`, `sensorhub.ko` (sensorhub SSP), `sprd_battery_info.ko`, `sprd-systimer.ko`.
> - **Tiga kelas bug "tidak pernah ter-build" yang diperbaiki** (semuanya silent, tanpa peringatan kconfig): (1) `SPRD_SYSTIMER` depends on `CLOCK_HRTIMER` — simbol yang **tidak ada** di pohon ini → diganti `HIGH_RES_TIMERS`; (2) `drivers/usb/typec/Makefile` tidak pernah descend ke `sprd/` → `sc27xx_typec.ko` mustahil lahir walau `CONFIG_TYPEC_SPRD=m`; (3) `sprdwcn` men-descend `wlan/` lewat simbol hantu `CONFIG_SPRD_LEGACY_WCN_WLAN` → diganti `CONFIG_WCN_WLAN` (+ `=m` di fragmen) sehingga `sprdwl.ko` benar-benar di-link.
> - **Deblob LEVEL 1 (NAME)**: **29/31** `.ko` stok punya padanan nama di build (sebelumnya 24/31). Sisa 2 — `sprd-vsp-pw-domain` dan `twofish_common` — bukan hilang: keduanya **built-in (`=y`) di vmlinux**, jadi secara fungsi 31/31 ada.
> - **Audit otomatis**: `donors/port_audit.py` mengklasifikasi 153 baris driver vendor vs pohon+build → hasil `docs/PORT-AUDIT.md` (KO 69, Y 47, OFF-DESIGN 2, NOSRC 34). Bukti build: `docs/evidence-20261023/{ko_list.txt,config.final}`.
> - **Batas jujur TICK12**: (1) belum boot/insmod di hardware; (2) `sprd_sensor` sengaja OFF (`sprd_cam_pw_*` belum ada), `arc4` OFF-by-design (butuh API crypto usermode + algoritma obsolete); (3) 34 baris vendor masih NOSRC (sumber tidak ada di pohon — butuh tulis driver / RE lanjut); (4) workflow GitHub Actions masih memakai guard yang terlalu ketat (416 simbol hidden/auto dilaporkan sebagai DROPPED) — build otoritatif sekarang di codespace.
>
> **Status kompilasi 2026-10-15 (TICK10 — thermal DT chain diproses 1/4, sprd_vdsp wiring fix, jujur, tanpa klaim berlebih):**
> - **TICK9 §4 thermal DT chain — DIPROSES 1/4.** `pmic_tsensor: tsensor@1b34` (parent + `osc-tsensor@0` + `tout-tsensor@1`) ditulis di `sc2730.dtsi`; `osctsen-thmzone` + `outtsen-thmzone` thermal zones ditulis di `ums512-1h10-vxtux.dts` (DTB stok `fdt.dts:5869-5881`). Header `sc2730.dtsi` dikoreksi: komentar lama "tsensor belum punya driver mainline" USANG (driver `sc27xx_tsensor_thermal.ko` ter-build 2026-10-13). Bukti: `docs/DEBLOB-20261015-TICK10-STATUS.md` §1.1. **Batas jujur**: node ini cuma membuka `sc27xx_tsensor` BISA bind; probe masih exit early kalau `/chosen/bootargs` tidak memuat `androidboot.mode=cali` (perilaku blob vendor). Sisa 3/4 = `ap_efuse@32240000` + 13 sel kalibrasi + 3 `ap_thm0/1/2` (thermal@32200000/32210000/32220000) + `virtual_sensor@0` + 10 zona thermal — TICK11.
> - **TICK7 retro-fix sprd_vdsp akhirnya di-commit.** TICK7 commit `4152b927f` mengaku vdsp "ported" dan README baris #89/181 menjelaskan `drivers/vxtux/npu/vdsp/sprd_vdsp_main.c` ada — faktanya file + wiring TIDAK pernah di-`git add` sampai tick ini. Yang masuk `.git`: `drivers/vxtux/npu/vdsp/{Kconfig,Makefile,ion.h}` (90 baris) + `common/{sprd_vdsp_mm.h,vdsp_trusty.h}` (240 baris) + `Cadence/xrp/*.{c,h}` (7.860 baris .c + 14 .h) + `drivers/devfreq/sprd/apsys/{sprd_dvfs_vdsp.h,vdsp_dvfs_sharkl5pro.h}` (186 baris) + wiring `drivers/vxtux/{Kconfig,Makefile}` (`source "drivers/vxtux/npu/vdsp/Kconfig"`, `obj-$(CONFIG_SPRD_VDSP) += npu/vdsp/`) + `CONFIG_SPRD_VDSP=m` di fragmen + `sprd_iommu_*_with_idx` exports di `drivers/iommu/sprd-iommu.c` (24 baris). Fragmen juga dibersihkan: `CONFIG_VIDEO_GC5025=y` duplikat dihapus (sudah aktif di baris `:935`), blok `=== VXTUX-BRINGUP-BEGIN ===` (69 baris CONFIG_BT/CFG80211/MEDIA_SUPPORT/VIDEO_GC8034) dihapus karena duplikat dengan baris lain di bawahnya. Bukti: `docs/DEBLOB-20261015-TICK10-STATUS.md` §1.2. **Batas jujur**: vdsp belum di-build di device tick ini; build loop untuk `make M=drivers/vxtux/npu/vdsp` menunggu device sidang.
> - **Tally 103/103** (TICK7 → sekarang, tidak bergerak): `work/pull/artifacts.txt` 13.958 baris, 0 MISS, 5 `.ko` heuristik + 98 `.o`. Kernel Image **50.473.472 byte** (de-bloat, TICK8). Versi rilis: `Linux version 6.18.52-VXTux-UMS512`.
> - **13 modul compile-verified device** (TICK9 → TICK10, tidak bergerak; TICK7 sprd_vdsp.ko llvm-nm **913.568 byte**). sprd-gsp + novatek_ts_spi TETAP dari TICK9 (`docs/DEBLOB-20261015-TICK9-STATUS.md`).
> - **System.map segar** (TICK9, tidak bergerak): `work/sysmaps/System.map.builddmed.20261014_2218` (md5 `82055ecc...`, 9.458.636 B) — `sprd_flash_cfg/ctrl/get_info`, `sprd_spi_probe`, `sprd_spi_of_match`, `sprd_spi_driver_init` semua ada.
> - **Image de-bloat 50.5 MB** (TICK8, tidak bergerak): +40 opsi `CONFIG_DEBUG_*` / `TRACING` (LOCKDEP, PROVE_LOCKING, DEBUG_OBJECTS×6, DEBUG_VM, …) yang masuk dari restore config sesi t28 sudah dimatikan. Sisanya ≈3,3 MB = 118 driver `=m→=y` yang sengaja dipertahankan (fitur, bukan kebocoran).
> - **Push `test1` ke origin `archanaberry/advan-vxtux-6.18.52`** (TICK10): branch dari main lokal (`69acfaacf`) + TICK8 `4152b927f` + TICK9 `e7cef58d3` + TICK9 addendum `a4181022a` + TICK10 (commit baru) — total 5+ commit ahead of `origin/main`. PR-ready: https://github.com/archanaberry/advan-vxtux-6.18.52/pull/new/test1
> - **Yang masih ditunda, jujurnya**: (1) verifikasi runtime `insmod` 13 modul di hardware — device sedang sidang, hanya kompilasi+link yang terbukti; (2) resync 1.435 `.ko` ke config de-bloat — wajib sebelum insmod/flash; (3) Image belum di-flash/boot-test; (4) **thermal DT chain masih 3/4** (TICK11): `ap_efuse` + 13 sel kalibrasi + 3 `ap_thm0/1/2` + `virtual_sensor` node; (5) **vdsp build verifikasi** di device (TICK10 retro-fix) — `make M=drivers/vxtux/npu/vdsp` baru akan jadi `sprd_vdsp.ko` `.config` actual (TICK7 angka 913.568 byte dari config pra-debloat mtime 09:40 WIB).

---

## 📋 Inventory per entri | Full 103 List desu~

| # | Ledger ID | Source in this tree | Status / evidence | Vendor 129 |
| ---: | --- | --- | --- | --- |
| 1 | `aes-ce-ccm` | [arch/arm64/crypto/aes-ce-ccm-glue.c](6.18.52-VXTux-UMS512/arch/arm64/crypto/aes-ce-ccm-glue.c) | Mainline implementation plus ARM64 glue  `[V] aes-ce-ccm` |
| 2 | `aes-neon-blk` | [arch/arm64/crypto/aes-glue-neon.c](6.18.52-VXTux-UMS512/arch/arm64/crypto/aes-glue-neon.c) | Mainline; module form in ledger  `[V] aes-neon-blk` |
| 3 | `agsd` | [drivers/sound/soc/sprd/agdsp_access/agdsp_access.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/agdsp_access/agdsp_access.c) | Re-mapped; Tier A/B evidence  `[V] agdsp_access` |
| 4 | `apsys-dvfs` | [drivers/devfreq/sprd/apsys/sprd_dvfs_apsys.c](6.18.52-VXTux-UMS512/drivers/devfreq/sprd/apsys/sprd_dvfs_apsys.c) | SPRD devfreq port  `[V] apsys-dvfs` |
| 5 | `arc4` | [crypto/arc4.c](6.18.52-VXTux-UMS512/crypto/arc4.c) | Mainline; ADA-verified in ledger  `[V] arc4` |
| 6 | `asix` | [drivers/net/usb/asix_common.c](6.18.52-VXTux-UMS512/drivers/net/usb/asix_common.c) | Mainline USB Ethernet; module  `[V] asix` |
| 7 | `audio-dsp-dump` | [drivers/sound/soc/sprd/audiodspdump/audio_dsp_dump.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audiodspdump/audio_dsp_dump.c) | SPRD port; module  `[V] audio-dsp-dump` |
| 8 | `audio-pipe` | [drivers/sound/soc/sprd/audiosipc/audio-pipe.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audiosipc/audio-pipe.c) | CONFIG SPRD_AUDIO_PIPE=y + DT node; runtime pending  `[V] audio-pipe` |
| 9 | `audio_mem` | [drivers/sound/soc/sprd/audiomem/audio_mem.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audiomem/audio_mem.c) | Path moved from donor layout; GPL fork source  `[V] audio_mem` |
| 10 | `audio_sipc` | [drivers/sound/soc/sprd/audiosipc/audio-sipc.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audiosipc/audio-sipc.c) | GPL fork port  `[V] audio_sipc` |
| 11 | `ax88179_178a` | [drivers/net/usb/ax88179_178a.c](6.18.52-VXTux-UMS512/drivers/net/usb/ax88179_178a.c) | Mainline USB Ethernet; module  `[V] ax88179_178a` |
| 12 | `bq2560x-charger` | [drivers/power/supply/bq256xx_charger.c](6.18.52-VXTux-UMS512/drivers/power/supply/bq256xx_charger.c) | CHARGER_BQ256XX=y; alias ti,bq2560x_chg verified in of_match + DT charger@6b  `[V] bq2560x-charger` |
| 13 | `core` | [drivers/net/wireless/sprd/sprdwcn/platform/Makefile](6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/platform/Makefile) | Composite `wcn_core` module; no standalone `core.c`. **Compile terverifikasi device: `wcn_core.ko`**  `[V] core, wcn_core` |
| 14 | `cpufreq_userspace` | [drivers/cpufreq/cpufreq_userspace.c](6.18.52-VXTux-UMS512/drivers/cpufreq/cpufreq_userspace.c) | Mainline  `[V] cpufreq_userspace` |
| 15 | `extcon-usb-gpio` | [drivers/extcon/extcon-usb-gpio.c](6.18.52-VXTux-UMS512/drivers/extcon/extcon-usb-gpio.c) | Mainline  `[V] extcon-usb-gpio` |
| 16 | `ghash-ce` | [arch/arm64/crypto/ghash-ce-glue.c](6.18.52-VXTux-UMS512/arch/arm64/crypto/ghash-ce-glue.c) | Mainline implementation plus ARM64 glue  `[V] ghash-ce` |
| 17 | `gnss_common_ctl_all` | [drivers/net/wireless/sprd/sprdwcn/platform/gnss/gnss_common_ctl.c](6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/platform/gnss/gnss_common_ctl.c) | Member of composite `wcn_core`  `[V] gnss_common_ctl_all` |
| 18 | `gnss_dbg` | [drivers/net/wireless/sprd/sprdwcn/platform/gnss/gnss_dbg.c](6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/platform/gnss/gnss_dbg.c) | Member of composite `wcn_core`  `[V] gnss_dbg` |
| 19 | `gnss_pmnotify_ctl` | [drivers/net/wireless/sprd/sprdwcn/platform/gnss/gnss_pmnotify_ctl.c](6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/platform/gnss/gnss_pmnotify_ctl.c) | Member of composite `wcn_core`  `[V] gnss_pmnotify_ctl` |
| 20 | `gpio` | [drivers/gpio/gpio-sprd.c](6.18.52-VXTux-UMS512/drivers/gpio/gpio-sprd.c) | Tier A  `[V] gpio, gpio-sprd` |
| 21 | `hxchipset_83102e` | [drivers/input/touchscreen/sprd/hx83102e/hxchipset/himax_platform.c](6.18.52-VXTux-UMS512/drivers/input/touchscreen/sprd/hx83102e/hxchipset/himax_platform.c) | Himax public-tree source  `[V] hxchipset_83102e` |
| 22 | `ims_bridge` | [net/ims_bridge/Makefile](6.18.52-VXTux-UMS512/net/ims_bridge/Makefile) | Tier A; composite network module  `[V] ims_bridge` |
| 23 | `ion_cma_heap` | [drivers/dma-buf/heaps/cma_heap.c](6.18.52-VXTux-UMS512/drivers/dma-buf/heaps/cma_heap.c) | DMABUF_HEAPS_CMA=y; mainline cma_heap  `[V] ion_cma_heap` |
| 24 | `ion_ipc_trusty` | [drivers/dma-buf/sprd/ion_ipc_trusty.c](6.18.52-VXTux-UMS512/drivers/dma-buf/sprd/ion_ipc_trusty.c) | Noble substitute; ported 2026-10-07  `[V] ion_ipc_trusty` |
| 25 | `jpg` | [drivers/misc/sprd_jpg/sprd_jpg_main.c](6.18.52-VXTux-UMS512/drivers/misc/sprd_jpg/sprd_jpg_main.c) | SPRD JPEG port (renamed from sprd_jpg.c — nama modul ≠ nama objek). **Compile terverifikasi device: `sprd_jpg.ko` 156 KB (gcc 16.2.1)** — GPL, depends sprd-iommu+sprd_ion. Tuntas 2026-10-10  `[V] jpg` |
| 26 | `leds-sc27xx-bltc` | [drivers/leds/leds-sc27xx-bltc.c](6.18.52-VXTux-UMS512/drivers/leds/leds-sc27xx-bltc.c) | LEDS_SC27XX_BLTC=y + DT sc2730-bltc node  `[V] leds-sc27xx-bltc` |
| 27 | `ledtrig-pattern` | [drivers/leds/trigger/ledtrig-pattern.c](6.18.52-VXTux-UMS512/drivers/leds/trigger/ledtrig-pattern.c) | LEDS_TRIGGER_PATTERN=y mainline  `[V] ledtrig-pattern` |
| 28 | `mcdt_hw` | [drivers/sound/soc/sprd/mcdt/mcdt_r1p0/mcdt_hw.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/mcdt/mcdt_r1p0/mcdt_hw.c) | Resolved through `sprd_mcdt_*` symbols  `[V] mcdt_hw` |
| 29 | `mipi_driver` | [drivers/unisoc_platform/debug_log/sharkl5/mipi_driver.c](6.18.52-VXTux-UMS512/drivers/unisoc_platform/debug_log/sharkl5/mipi_driver.c) | Noble substitute via radare2+ghidra (Tier B). **Stub saja — TIDAK ikut dibangun**: `sharkl5/` tidak punya Makefile, jadi berkas ini bukti niat, bukan bukti build. Modul yang benar-benar dibangun = trio inti di `debug_log/` (lihat #81)  `[V] mipi_driver` |
| 30 | `misc_sprd_uid` | [drivers/soc/sprd/soc_id/sprd_uid.c](6.18.52-VXTux-UMS512/drivers/soc/sprd/soc_id/sprd_uid.c) | Tier A  `[V] misc_sprd_uid` |
| 31 | `musb_hdrc` | [drivers/usb/musb/musb_core.c](6.18.52-VXTux-UMS512/drivers/usb/musb/musb_core.c) | USB_MUSB_HDRC=y verified 2026-10-09  `[V] musb_hdrc` |
| 32 | `musb_sprd` | [drivers/usb/musb/musb_sprd_vxtux.c](6.18.52-VXTux-UMS512/drivers/usb/musb/musb_sprd_vxtux.c) | USB_MUSB_SPRD_VXTUX=y added 2026-10-09; of_match + DT node aligned  `[V] musb_sprd` |
| 33 | `novatek_ts_spi` | [drivers/input/touchscreen/novatek_nvt_ts_spi/nt36xxx.c](6.18.52-VXTux-UMS512/drivers/input/touchscreen/novatek_nvt_ts_spi/nt36xxx.c) | **Compile terverifikasi device (TICK9, 14 Okt 09:40 WIB)**: `novatek_ts_spi.ko` **512.584 byte**, llvm-nm **337 simbol terdefinisi** / 88 undefined. Modul komposit 4 objek (`nt36xxx.o nt36xxx_fw_update.o nt36xxx_ext_proc.o nt36xxx_mp_ctrlram.o`). Simbol kunci: `T CTP_SPI_READ/WRITE`, `T nvt_set_page`, `T nvt_sw_reset`, `T nvt_bootloader_reset`, `T nvt_esd_check_enable`, `T nvt_mp_proc_init`, `T nvt_extra_proc_init`, `T nvt_update_firmware` — keluarga sama dengan blob stok `blobs/re_notes/novatek_ts_spi.sym`. DT: `touchscreen@0` compatible `"novatek,NVT-ts-spi"` terverifikasi di DTB (dtc decompile baris 1566-1577) cocok `nvt_match_table` (nt36xxx.c:2407). `CONFIG_TOUCHSCREEN_NOVATEK_NVT_TS_SPI=m`. Tuntas 2026-10-13 (driver SPI 7.420 baris, port nt36xxx), build TICK9.  `[V] novatek_ts_spi` |
| 34 | `phy-sprd-sharkl5Pro` | [drivers/phy/sprd/phy-sprd-ums512.c](6.18.52-VXTux-UMS512/drivers/phy/sprd/phy-sprd-ums512.c) | Tier A  `[V] phy-sprd-sharkl5Pro` |
| 35 | `pinctrl` | [drivers/pinctrl/sprd/pinctrl-sprd.c](6.18.52-VXTux-UMS512/drivers/pinctrl/sprd/pinctrl-sprd.c) | Tier A; shared source with next ledger entry  `[V] pinctrl, pinctrl-sprd` |
| 36 | `pinctrl-sprd` | [drivers/pinctrl/sprd/pinctrl-sprd.c](6.18.52-VXTux-UMS512/drivers/pinctrl/sprd/pinctrl-sprd.c) | Tier A; same source path as previous entry  `[V] pinctrl-sprd` |
| 37 | `pinctrl-sprd-sharkl5Pro` | [drivers/pinctrl/sprd/pinctrl-sprd-sharkl5pro.c](6.18.52-VXTux-UMS512/drivers/pinctrl/sprd/pinctrl-sprd-sharkl5pro.c) | Tier A  `[V] pinctrl-sprd-sharkl5Pro` |
| 38 | `pwm-sprd` | [drivers/pwm/pwm-sprd.c](6.18.52-VXTux-UMS512/drivers/pwm/pwm-sprd.c) | Tier A  `[V] pwm-sprd` |
| 39 | `pwm_bl` | [drivers/video/backlight/pwm_bl.c](6.18.52-VXTux-UMS512/drivers/video/backlight/pwm_bl.c) | Mainline PWM backlight  `[V] pwm_bl` |
| 40 | `rtc-sc27xx` | [drivers/rtc/rtc-sc27xx.c](6.18.52-VXTux-UMS512/drivers/rtc/rtc-sc27xx.c) | Tier A  `[V] rtc-sc27xx` |
| 41 | `sc27xx-poweroff` | [drivers/power/reset/sc27xx-poweroff.c](6.18.52-VXTux-UMS512/drivers/power/reset/sc27xx-poweroff.c) | Tier A  `[V] sc27xx-poweroff` |
| 42 | `sc27xx-vibra` | [drivers/input/misc/sc27xx-vibra.c](6.18.52-VXTux-UMS512/drivers/input/misc/sc27xx-vibra.c) | INPUT_SC27XX_VIBRA=y + DT sc2730-vibrator  `[V] sc27xx-vibra` |
| 43 | `sc27xx_adc` | [drivers/iio/adc/sc27xx_adc.c](6.18.52-VXTux-UMS512/drivers/iio/adc/sc27xx_adc.c) | Tier A  `[V] sc27xx_adc` |
| 44 | `sc27xx_fuel_gauge` | [drivers/power/supply/sc27xx_fuel_gauge.c](6.18.52-VXTux-UMS512/drivers/power/supply/sc27xx_fuel_gauge.c) | Tier A  `[V] sc27xx_fuel_gauge` |
| 45 | `sc27xx_tsensor_thermal` | [drivers/thermal/sprd/sc27xx_tsensor_thermal.c](6.18.52-VXTux-UMS512/drivers/thermal/sprd/sc27xx_tsensor_thermal.c) | Tier A `[V] CONFIG_SPRD_TSENSOR_THERMAL=y (fragmen L1204); sumber di tree, build 14 Okt` |
| 46 | `sc27xx_typec` | [drivers/usb/typec/sprd/sc27xx_typec.c](6.18.52-VXTux-UMS512/drivers/usb/typec/sprd/sc27xx_typec.c) | Mainline-based port; module  `[V] sc27xx_typec` |
| 47 | `seth` | [drivers/soc/sprd/sipc/seth.c](6.18.52-VXTux-UMS512/drivers/soc/sprd/sipc/seth.c) | Tier A  `[V] seth` |
| 48 | `sha1-ce` | [crypto/sha1.c](6.18.52-VXTux-UMS512/crypto/sha1.c) | Mainline  `[V] sha1-ce` |
| 49 | `snd-soc-sprd-card` | [drivers/sound/soc/sprd/card/sprd-asoc-card-utils.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/card/sprd-asoc-card-utils.c) | Tier A  `[V] snd-soc-sprd-card` |
| 50 | `snd-soc-sprd-codec-sc2730` | [drivers/sound/soc/sprd/codec_sc2730/sprd-codec.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/codec_sc2730/sprd-codec.c) | Tier A  `[V] snd-soc-sprd-codec-sc2730` |
| 51 | `snd-soc-sprd-codec-sc2730-power` | [drivers/sound/soc/sprd/codec/sprd-audio-power.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/codec/sprd-audio-power.c) | Tier A; shared source with next entry  `[V] snd-soc-sprd-codec-sc2730-power` |
| 52 | `snd-soc-sprd-codec-sc2730-power-dev` | [drivers/sound/soc/sprd/codec/sprd-audio-power.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/codec/sprd-audio-power.c) | Tier A; same source path as previous entry  `[V] snd-soc-sprd-codec-sc2730-power-dev` |
| 53 | `snd-soc-sprd-dummy-codec` | [drivers/sound/soc/sprd/codec/dummy-codec.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/codec/dummy-codec.c) | Tier A  `[V] snd-soc-sprd-dummy-codec` |
| 54 | `snd-soc-sprd-vbc-fe` | [drivers/sound/soc/sprd/dai_v4/vbc-dai.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/dai_v4/vbc-dai.c) | VXTUX_SND_SPRD_VBC_V4=y + DT sharkl5-vbc  `[V] snd-soc-sprd-vbc-fe` |
| 55 | `snd-soc-sprd-vbc-v4` | [drivers/sound/soc/sprd/dai_v4/vbc-phy-v4.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/dai_v4/vbc-phy-v4.c) | VXTUX_SND_SPRD_VBC_V4=y + DT sharkl5-vbc  `[V] snd-soc-sprd-vbc-v4` |
| 56 | `spi-sprd` | [drivers/spi/spi-sprd.c](6.18.52-VXTux-UMS512/drivers/spi/spi-sprd.c) | **Closed via System.map (TICK9, 14 Okt 22:18 WIB)**: 23 simbol — `t sprd_spi_probe` (baris 48264), `d sprd_spi_of_match` (126790), `t sprd_spi_driver_init` (159529), `t sprd_spi_remove`. Tier A + System.map `work/sysmaps/System.map.builddmed.20261014_2218` (md5 `82055ecc...`).  `[V] spi-sprd` |
| 57 | `sprd-charger-manager` | [drivers/power/supply/sprd/sprd_charger_manager.c](6.18.52-VXTux-UMS512/drivers/power/supply/sprd/sprd_charger_manager.c) | Tier A  `[V] sprd-charger-manager` |
| 58 | `sprd-compr-2stage-dma` | [drivers/dma/sprd-dma.c](6.18.52-VXTux-UMS512/drivers/dma/sprd-dma.c) | Tier A; shares implementation with `sprd-dma`  `[V] sprd-compr-2stage-dma` |
| 59 | `sprd-dma` | [drivers/dma/sprd-dma.c](6.18.52-VXTux-UMS512/drivers/dma/sprd-dma.c) | Tier A; same source path as previous entry  `[V] sprd-dma` |
| 60 | `sprd-dmaengine-pcm` | [drivers/sound/soc/sprd/platform_include/sprd-dmaengine-pcm.h](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/platform_include/sprd-dmaengine-pcm.h) | Header-only; Tier B  `[V] sprd-dmaengine-pcm` |
| 61 | `sprd-drm` | [drivers/gpu/drm/sprd/sprd_drm.c](6.18.52-VXTux-UMS512/drivers/gpu/drm/sprd/sprd_drm.c) | Tier A  `[V] sprd-drm` |
| 62 | `sprd-gsp` | [drivers/gpu/drm/sprd/gsp/gsp_dev.c](6.18.52-VXTux-UMS512/drivers/gpu/drm/sprd/gsp/gsp_dev.c) | **Compile terverifikasi device (TICK9, 14 Okt 09:40 WIB)**: `sprd-gsp.ko` **767.488 byte** (md5 `bfefe9af...`), llvm-nm **535 simbol terdefinisi** / 111 undefined (semua kernel). Simbol kunci: `t gsp_dev_probe`, `t gsp_dev_remove`, `T gsp_core_init`, `T gsp_core_parse_dt`, `d gsp_dev_pmops`, `r gsp_component_ops`. **Wiring diperbaiki 2026-10-10**: dulu satu baris `obj-$(CONFIG_DRM_SPRD_GSP) += 9 objek` = sembilan modul terpisah yang saling menunjuk simbol `gsp_core_*` yang tidak di-export (modpost: undefined) — kini satu modul komposit `sprd-gsp.ko` (pola sama dengan vsp). Klaim tick-13 "0 dari 45 fungsi gsp_dev.c terhubung" DICABUT di TICK9. Catatan jujur: `struct gsp_cfg` direkonstruksi dari offset blob — kebenaran layout struct TIDAK terbukti (kalau salah, driver bisa probe lalu korup layer).  `[V] sprd-gsp` |
| 63 | `sprd-ion` | [drivers/dma-buf/sprd/sprd_ion.c](6.18.52-VXTux-UMS512/drivers/dma-buf/sprd/sprd_ion.c) | Tier A; **Compile terverifikasi device: `sprd_ion.ko` 62 KB (gcc 16.2.1)** — fix build: `CONFIG_SPRD_ION` dideklarasi (undeclared → olddefconfig drop), `MODULE_IMPORT_NS(DMA_BUF)`, `struct ion_phy_data` direstore  `[V] sprd-ion` |
| 64 | `sprd-platform-pcm-routing` | [drivers/sound/soc/sprd/platform/sprd-platform-pcm-routing.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/platform/sprd-platform-pcm-routing.c) | Tier A  `[V] sprd-platform-pcm-routing` |
| 65 | `sprd-top-dvfs` | [drivers/devfreq/sprd/sprd-top-dvfs.c](6.18.52-VXTux-UMS512/drivers/devfreq/sprd/sprd-top-dvfs.c) | Tier A `[V-ALT] apsys-dvfs: vendor 1 modul apsys-dvfs.ko dipecah jadi 3 of_match (top/cp/ddr) di port. CONFIG_SPRD_TOP_DVFS=m di fragmen` |
| 66 | `sprd-vsp-pw-domain` | [drivers/pmdomain/sprd/sprd_vsp_pw_domain.c](6.18.52-VXTux-UMS512/drivers/pmdomain/sprd/sprd_vsp_pw_domain.c) | VSP power-domain gate (library-style, tanpa DT power-domains); **Compile terverifikasi device: `sprd_vsp_pw_domain.ko` 52 KB (gcc 16.2.1)**  `[V] sprd-vsp-pw-domain` |
| 67 | `sprd_apipe` | [drivers/sound/soc/sprd/audio_pipe/audio_pipe.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audio_pipe/audio_pipe.c) | Port of older driver  `[V] sprd_apipe` |
| 68 | `sprd_audcp_boot` | [drivers/sound/soc/sprd/audiocpboot/sprd_audcp_boot.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audiocpboot/sprd_audcp_boot.c) | Tier A/B symbol evidence  `[V] sprd_audcp_boot` |
| 69 | `sprd_audcp_dvfs` | [drivers/sound/soc/sprd/audiodvfs/sprd_audcp_dvfs.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audiodvfs/sprd_audcp_dvfs.c) | Compiled 2026-10-04  `[V] sprd_audcp_dvfs` |
| 70 | `sprd_battery_info` | [drivers/power/supply/sprd/sprd_battery_info_main.c](6.18.52-VXTux-UMS512/drivers/power/supply/sprd/sprd_battery_info_main.c) | **Compile terverifikasi device: `sprd_battery_info.ko` 96 KB (gcc 16.2.1)** — `fgauge_get_profile_id()` di-port dari donor `unisoc_battery.c` (baru `unisoc_battery_id.c`); DT satu-profil → fallback donor: id 0. Dulu =n ("INCOMPLETE"), tuntas 2026-10-09  `[V] sprd_battery_info` |
| 71 | `sprd_camera` | [drivers/media/platform/sprd/pipeline/sprd_dcam.c](6.18.52-VXTux-UMS512/drivers/media/platform/sprd/pipeline/sprd_dcam.c) | VIDEO_SPRD_DCAM=y; camera graph fixed 2026-10-09 (was non-bidirectional)  `[V] sprd_camera` |
| 72 | `sprd_camsys_pw_domain` | [drivers/pmdomain/sprd/sprd_camsys_pw_domain.c](6.18.52-VXTux-UMS512/drivers/pmdomain/sprd/sprd_camsys_pw_domain.c) | Power-domain port (platform_driver asli); **Compile terverifikasi device: `sprd_camsys_pw_domain.ko` 95 KB (gcc 16.2.1)**  `[V] sprd_camsys_pw_domain` |
| 73 | `sprd_cp_dvfs` | [drivers/devfreq/sprd/apsys/sprd_dvfs_apsys.c](6.18.52-VXTux-UMS512/drivers/devfreq/sprd/apsys/sprd_dvfs_apsys.c) | Same object as `apsys-dvfs` `[V] sprd_audcp_dvfs: vendor sprd_cp_dvfs = sprd_audcp_dvfs.ko (alias dari apsys-dvfs vendor). Entry ledger ambigu, fix: rename ke sprd_audcp_dvfs (alias of_match ke-2 apsys-dvfs)` |
| 74 | `sprd_cpp` | [drivers/media/platform/sprd/cpp/cpp_core.c](6.18.52-VXTux-UMS512/drivers/media/platform/sprd/cpp/cpp_core.c) | SPRD_CPP=y + DT cpp@62800000; ported 2026-10-09 (13 file vendor). **Compile terverifikasi device: `cpp.ko` 321 KB (gcc 16.2.1)**  `[V] sprd_cpp` |
| 75 | `sprd_cpu_cooling` | [drivers/thermal/sprd/sprd_cpu_cooling.c](6.18.52-VXTux-UMS512/drivers/thermal/sprd/sprd_cpu_cooling.c) | Tier A `[V] CONFIG_SPRD_CPU_COOLING=y (fragmen L1223); late_initcall, tanpa of_match_table (model vendor)` |
| 76 | `sprd_ddr_dvfs` | [drivers/devfreq/sprd/apsys/sprd_dvfs_apsys.c](6.18.52-VXTux-UMS512/drivers/devfreq/sprd/apsys/sprd_dvfs_apsys.c) | Same object as `apsys-dvfs` `[V-ALT] apsys-dvfs: entry D obsolete, sumber engga ada. Diselamatkan sebagai alias of_match ke-3 apsys-dvfs (rename dari sprd_ddr_dvfs jadi apsys-dvfs alias)` |
| 77 | `sprd_flash_drv` | [drivers/sprd/flash/flash_drv.c](6.18.52-VXTux-UMS512/drivers/sprd/flash/flash_drv.c) | **Closed via System.map (TICK9, 14 Okt 22:18 WIB)**: 14 simbol — `T sprd_flash_ctrl`, `t sprd_flash_probe`, `T sprd_flash_register`, `r __ksymtab_sprd_flash_*` (CFG/ctrl/get_info baris 923-925, register/probe 68359/68361). Tier A + System.map `work/sysmaps/System.map.builddmed.20261014_2218` (md5 `82055ecc...`).  `[V] sprd_flash_drv` |
| 78 | `sprd_fm` | [drivers/net/wireless/sprd/sprdwcn/fm/driver/fm_sdio/fmdrv_main.c](6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/fm/driver/fm_sdio/fmdrv_main.c) | Tier A; 4/4 objects, zero compile errors per ledger  `[V] sprd_fm` |
| 79 | `sprd_gpu_cooling` | [drivers/thermal/sprd/sprd_gpu_cooling.c](6.18.52-VXTux-UMS512/drivers/thermal/sprd/sprd_gpu_cooling.c) | Tier A `[V] CONFIG_VXTUX_SPRD_GPU_COOLING=y (fragmen L310); =y built-in, ENGGAAA jadi .ko` |
| 80 | `sprd_map` | [drivers/misc/sprd/sprd_map.c](6.18.52-VXTux-UMS512/drivers/misc/sprd/sprd_map.c) | Tier A `[V] CONFIG_SPRD_MAP=y (fragmen L253); sumber ION/System.map equivalent, vendor ENGGAAA punya` |
| 81 | `sprd_mipi` | [drivers/unisoc_platform/debug_log/Makefile](6.18.52-VXTux-UMS512/drivers/unisoc_platform/debug_log/Makefile) | Modem debug-log core, diport 2026-10-10 dari donor `bocchi_unisoc54` (`core.c` + `sysfs.c` + `serdes.c`); modul komposit `sprd_mipi.ko`, `CONFIG_SPRD_MODEM_DEBUG=y`. Ekspor: `dbg_log_device_register`, `dbg_log_sysfs_init`, `serdes_enable`. Varian per-SoC `sharkl5/` belum diport. Kompilasi menunggu build device  `[V] sprd_mipi` |
| 82 | `sprd_pmic_syscon` | [drivers/soc/sprd/pmic_syscon/sprd_pmic_syscon.c](6.18.52-VXTux-UMS512/drivers/soc/sprd/pmic_syscon/sprd_pmic_syscon.c) | Tier A  `[V] sprd_pmic_syscon` |
| 83 | `sprd_pmic_wdt` | [drivers/watchdog/sprd_wdt.c](6.18.52-VXTux-UMS512/drivers/watchdog/sprd_wdt.c) | Linked by symbol-family evidence `[V] sprd_wdt_fiq: entry D obsolete, sumber engga ada. Diselamatkan: diganti `sprd_wdt_fiq` (drivers/watchdog/sprd_wdt_fiq.c ADA di tree, CONFIG_SPRD_WATCHDOG_FIQ=y)` |
| 84 | `sprd_sensor` | [drivers/media/platform/sprd/sensors/sprd_sensor/Makefile](6.18.52-VXTux-UMS512/drivers/media/platform/sprd/sensors/sprd_sensor/Makefile) | Tier A sensor framework; multiple source files  `[V] sprd_sensor` |
| 85 | `sprd_soc_thm` | [drivers/thermal/sprd/sprd_soc_thm.c](6.18.52-VXTux-UMS512/drivers/thermal/sprd/sprd_soc_thm.c) | Tier A  `[V] sprd_soc_thm` |
| 86 | `sprd_thermal` | [drivers/thermal/sprd_thermal.c](6.18.52-VXTux-UMS512/drivers/thermal/sprd_thermal.c) | Tier A `[V-TICK11+] BUILTIN=y` (nunggu thermal DT chain 2/4, 3/4, 4/4: ap_efuse + 13 sel kalibrasi + 3 ap_thm + virtual_sensor) |
| 87 | `sprd_u_ether` | [drivers/usb/gadget/function/sprd/sprd_u_ether.c](6.18.52-VXTux-UMS512/drivers/usb/gadget/function/sprd/sprd_u_ether.c) | Tier A `[V] CONFIG_USB_GADGET_SPRD=y (fragmen L659, aggregator sprd_u_ether + sprd_usb_f_rndis); vendor ENGGAAA aktifkan USB gadget` |
| 88 | `sprd_usb_f_rndis` | [drivers/usb/gadget/function/sprd/sprd_usb_f_rndis.c](6.18.52-VXTux-UMS512/drivers/usb/gadget/function/sprd/sprd_usb_f_rndis.c) | Tier A `[V] CONFIG_USB_GADGET_SPRD=y (fragmen L659, aggregator sprd_u_ether + sprd_usb_f_rndis); vendor ENGGAAA aktifkan` |
| 89 | `sprd_vdsp` | [drivers/vxtux/npu/vdsp/sprd_vdsp_main.c](6.18.52-VXTux-UMS512/drivers/vxtux/npu/vdsp/sprd_vdsp_main.c) | **Ported 2026-10-14** dari donor 4.14 (`iscle_ums512` lewat `work/donorcatalog/mods-sprd` — 31 file Cadence XRP). 14 objek `.o` di-build, total `sprd_vdsp.ko` **913.568 byte** (relink penuh, log device `t21_full.log` 10:24 UTC). Driver aslinya di kernel5.4 stock `npu/vdsp/` (iommu/mem/ipi/xvp symbols) cocok 100% dengan `.sym` — header dvfs (`sprd_dvfs_vdsp.h`, `vdsp_dvfs_sharkl5pro.h`) ditarik via curl dari raw iscle. Fix build: `elf.h` donor (bentrok dengan `uapi/linux/elf.h` → typedef 22 error) di-strip 7 struct + 5 macro-typedef; ABI 4.14→6.18 (`ida_simple_get/remove` → `ida_alloc_range/ida_free`, `no_llseek` → `default_llseek`, `vfree` include, `.remove` void); shim ion `vdsp_ion_free()` (donor panggil `ion_free(dmabuf)`, shim tree 6.18 `ion_free` butuh handle); `MODULE_IMPORT_NS("DMA_BUF")`; `sprd_iommu_map/unmap` (varian `_with_idx` tidak terekspor di `Module.symvers` 6.18). Runtime: insmod menunggu device sidang.  `[V] sprd_vdsp` |
| 90 | `sprd_wdf` | [drivers/watchdog/sprd_wdf.c](6.18.52-VXTux-UMS512/drivers/watchdog/sprd_wdf.c) | Tier A  `[V] sprd_wdf` |
| 91 | `sprd_wlan_combo` | [drivers/net/wireless/sprd/sprdwcn/wlan/Makefile](6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/wlan/Makefile) | SC2355 Wi-Fi port (27 objek → modul `sprdwl`). **Compile terverifikasi device: `sprdwl.ko` 2,5 MB (gcc 16.2.1)** — GPL, depends `cfg80211+wcn_core`, of_match `sprd,sc2355-wifi` (WiFi + SoftAP/hotspot). Tuntas 2026-10-09  `[V] sprdwl, sprd_wlan_combo` |
| 92 | `sprdbt_tty` | [drivers/net/wireless/sprd/sprdwcn/bluetooth/driver/Makefile](6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/bluetooth/driver/Makefile) | HCI over tty-SDIO; composite module  `[V] sprdbt_tty` |
| 93 | `thermal-generic-adc` | [drivers/thermal/thermal-generic-adc.c](6.18.52-VXTux-UMS512/drivers/thermal/thermal-generic-adc.c) | Mainline `[V-MAINLINE]` (mainline generic, bukan vendor-specific — dikonfirm `.ko` ada di kolist TICK7) |
| 94 | `trusty-tui` | [drivers/misc/trusty-tui.c](6.18.52-VXTux-UMS512/drivers/misc/trusty-tui.c) | Port of vendor `trusty-tui.ko`  `[V] trusty-tui, trusty, +5` |
| 95 | `twofish_common` | [crypto/twofish_common.c](6.18.52-VXTux-UMS512/crypto/twofish_common.c) | Mainline  `[V] twofish_common` |
| 96 | `twofish_generic` | [crypto/twofish_generic.c](6.18.52-VXTux-UMS512/crypto/twofish_generic.c) | Mainline  `[V] twofish_generic` |
| 97 | `unisoc-iommu` | [drivers/iommu/sprd-iommu.c](6.18.52-VXTux-UMS512/drivers/iommu/sprd-iommu.c) | Tier A; **Compile terverifikasi device: `sprd-iommu.ko` 81 KB (gcc 16.2.1)** — `sprd_iommu_restore` didefinisikan + di-export (sebelumnya declared-only → undefined di vsp/cpp); public API attach/map/unmap reimplemented di atas API generik  `[V] unisoc-iommu` |
| 98 | `virt-dma` | [drivers/dma/virt-dma.c](6.18.52-VXTux-UMS512/drivers/dma/virt-dma.c) | Tier A shared DMA helper  `[V] virt-dma` |
| 99 | `vsp` | [drivers/media/platform/sprd/vsp/sprd_vsp_main.c](6.18.52-VXTux-UMS512/drivers/media/platform/sprd/vsp/sprd_vsp_main.c) | SPRD_VSP=y + DT video-codec@20500000; ported 2026-10-09 from iscle_ums512 donor (pw-domain + regs + dvfs header sudah ada). **Compile terverifikasi device: `sprd_vsp.ko` 231 KB (gcc 16.2.1)**  `[V] vsp, sprd_vsp` |
| 100 | `wcn_bsp` | [drivers/net/wireless/sprd/sprdwcn/platform/Makefile](6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/platform/Makefile) | WCN platform module composition; no standalone `bsp.c` found  `[V] wcn_bsp, wcn_core` |
| 101 | `zram` | [drivers/block/zram/zram_drv.c](6.18.52-VXTux-UMS512/drivers/block/zram/zram_drv.c) | N/A/absence entry in the supplied tally; corrected 2026-10-05  `[V] zram` |
| 102 | `zsmalloc` | [mm/zsmalloc.c](6.18.52-VXTux-UMS512/mm/zsmalloc.c) | ZRAM=y + zsmalloc in mm/  `[V] zsmalloc` |
| 103 | `panfrost` | [drivers/gpu/drm/panfrost/panfrost_gpu.c](6.18.52-VXTux-UMS512/drivers/gpu/drm/panfrost/panfrost_gpu.c) | Separate open-source GPU driver; Mali-G52 MP2 / Gondul replacement `[V-MAINLINE]` (mainline GPU, REPLACES vendor Mali Gondul DDK — not vendor but TICK9+) |
| 104 | `core` | [drivers/core.c](6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/platform/core.c) | **Vendor feasible match** #14 (alias `core` dari vendor Advan `core.ko`, match dengan port 6.18) — WCN `[V-FEASIBLE]` |
| 105 | `wcn_bsp` | [drivers/wcn_bsp.c](6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/platform/wcn_bsp.c) | **Vendor feasible match** #100 (alias `wcn_bsp` dari vendor Advan `wcn_bsp.ko`, match dengan port 6.18) — WCN `[V-FEASIBLE]` |
| 106 | `wcn36xx` | [drivers/wcn36xx.c](6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/platform/wcn36xx.c) | **Vendor feasible match** #14 (alias `wcn36xx` dari vendor Advan `wcn36xx.ko`, match dengan port 6.18) — WCN (composite) `[V-FEASIBLE]` |
| 107 | `wcnss_ctrl` | [drivers/wcnss_ctrl.c](6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/platform/wcnss_ctrl.c) | **Vendor feasible match** #14 (alias `wcnss_ctrl` dari vendor Advan `wcnss_ctrl.ko`, match dengan port 6.18) — WCN (composite) `[V-FEASIBLE]` |
| 108 | `agdsp_access` | [drivers/agdsp_access.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/agdsp_access/agdsp_access.c) | **Vendor feasible match** #3 (alias `agdsp_access` dari vendor Advan `agdsp_access.ko`, match dengan port 6.18) — Audio `[V-FEASIBLE]` |
| 109 | `audio-dsp-dump` | [drivers/audio-dsp-dump.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audiodspdump/audio-dsp-dump.c) | **Vendor feasible match** #7 (alias `audio-dsp-dump` dari vendor Advan `audio-dsp-dump.ko`, match dengan port 6.18) — Audio `[V-FEASIBLE]` |
| 110 | `audio-pipe` | [drivers/audio-pipe.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audiosipc/audio-pipe.c) | **Vendor feasible match** #8 (alias `audio-pipe` dari vendor Advan `audio-pipe.ko`, match dengan port 6.18) — Audio `[V-FEASIBLE]` |
| 111 | `audio_mem` | [drivers/audio_mem.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audiomem/audio_mem.c) | **Vendor feasible match** #9 (alias `audio_mem` dari vendor Advan `audio_mem.ko`, match dengan port 6.18) — Audio `[V-FEASIBLE]` |
| 112 | `audio_sipc` | [drivers/audio_sipc.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audiosipc/audio_sipc.c) | **Vendor feasible match** #10 (alias `audio_sipc` dari vendor Advan `audio_sipc.ko`, match dengan port 6.18) — Audio `[V-FEASIBLE]` |
| 113 | `kfifo_buf` | [drivers/kfifo_buf.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/audiosipc/kfifo_buf.c) | **Vendor feasible match** #27 (alias `kfifo_buf` dari vendor Advan `kfifo_buf.ko`, match dengan port 6.18) — Audio `[V-FEASIBLE]` |
| 114 | `mcdt_hw` | [drivers/mcdt_hw.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/mcdt/mcdt_r1p0/mcdt_hw.c) | **Vendor feasible match** #30 (alias `mcdt_hw` dari vendor Advan `mcdt_hw.ko`, match dengan port 6.18) — Audio `[V-FEASIBLE]` |
| 115 | `snd-soc-sprd-card` | [drivers/snd-soc-sprd-card.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/card/snd-soc-sprd-card.c) | **Vendor feasible match** #49 (alias `snd-soc-sprd-card` dari vendor Advan `snd-soc-sprd-card.ko`, match dengan port 6.18) — Audio codec `[V-FEASIBLE]` |
| 116 | `snd-soc-sprd-codec-sc2730` | [drivers/snd-soc-sprd-codec-sc2730.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/codec_sc2730/snd-soc-sprd-codec-sc2730.c) | **Vendor feasible match** #50 (alias `snd-soc-sprd-codec-sc2730` dari vendor Advan `snd-soc-sprd-codec-sc2730.ko`, match dengan port 6.18) — Audio codec `[V-FEASIBLE]` |
| 117 | `snd-soc-sprd-codec-sc2730-power` | [drivers/snd-soc-sprd-codec-sc2730-power.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/codec/snd-soc-sprd-codec-sc2730-power.c) | **Vendor feasible match** #51 (alias `snd-soc-sprd-codec-sc2730-power` dari vendor Advan `snd-soc-sprd-codec-sc2730-power.ko`, match dengan port 6.18) — Audio codec `[V-FEASIBLE]` |
| 118 | `snd-soc-sprd-codec-sc2730-power-dev` | [drivers/snd-soc-sprd-codec-sc2730-power-dev.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/codec/snd-soc-sprd-codec-sc2730-power-dev.c) | **Vendor feasible match** #52 (alias `snd-soc-sprd-codec-sc2730-power-dev` dari vendor Advan `snd-soc-sprd-codec-sc2730-power-dev.ko`, match dengan port 6.18) — Audio codec `[V-FEASIBLE]` |
| 119 | `snd-soc-sprd-dummy-codec` | [drivers/snd-soc-sprd-dummy-codec.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/codec/dummy-codec.csnd-soc-sprd-dummy-codec.c) | **Vendor feasible match** #53 (alias `snd-soc-sprd-dummy-codec` dari vendor Advan `snd-soc-sprd-dummy-codec.ko`, match dengan port 6.18) — Audio codec `[V-FEASIBLE]` |
| 120 | `snd-soc-sprd-vbc-fe` | [drivers/snd-soc-sprd-vbc-fe.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/dai_v4/vbc-dai.csnd-soc-sprd-vbc-fe.c) | **Vendor feasible match** #54 (alias `snd-soc-sprd-vbc-fe` dari vendor Advan `snd-soc-sprd-vbc-fe.ko`, match dengan port 6.18) — Audio `[V-FEASIBLE]` |
| 121 | `snd-soc-sprd-vbc-v4` | [drivers/snd-soc-sprd-vbc-v4.c](6.18.52-VXTux-UMS512/drivers/sound/soc/sprd/dai_v4/vbc-phy-v4.csnd-soc-sprd-vbc-v4.c) | **Vendor feasible match** #55 (alias `snd-soc-sprd-vbc-v4` dari vendor Advan `snd-soc-sprd-vbc-v4.ko`, match dengan port 6.18) — Audio `[V-FEASIBLE]` |
| 122 | `bq2560x-charger` | [drivers/bq2560x-charger.c](6.18.52-VXTux-UMS512/drivers/power/supply/bq256xx_charger.cbq2560x-charger.c) | **Vendor feasible match** #12 (alias `bq2560x-charger` dari vendor Advan `bq2560x-charger.ko`, match dengan port 6.18) — Power `[V-FEASIBLE]` |
| 123 | `sc27xx_adc` | [drivers/sc27xx_adc.c](6.18.52-VXTux-UMS512/drivers/iio/adc/sc27xx_adc.csc27xx_adc.c) | **Vendor feasible match** #43 (alias `sc27xx_adc` dari vendor Advan `sc27xx_adc.ko`, match dengan port 6.18) — Sensor/ADC `[V-FEASIBLE]` |
| 124 | `sc27xx_fuel_gauge` | [drivers/sc27xx_fuel_gauge.c](6.18.52-VXTux-UMS512/drivers/power/supply/sc27xx_fuel_gauge.csc27xx_fuel_gauge.c) | **Vendor feasible match** #44 (alias `sc27xx_fuel_gauge` dari vendor Advan `sc27xx_fuel_gauge.ko`, match dengan port 6.18) — Power `[V-FEASIBLE]` |
| 125 | `sc27xx_typec` | [drivers/sc27xx_typec.c](6.18.52-VXTux-UMS512/drivers/usb/typec/sprd/sc27xx_typec.csc27xx_typec.c) | **Vendor feasible match** #46 (alias `sc27xx_typec` dari vendor Advan `sc27xx_typec.ko`, match dengan port 6.18) — Type-C `[V-FEASIBLE]` |
| 126 | `sc27xx-poweroff` | [drivers/sc27xx-poweroff.c](6.18.52-VXTux-UMS512/drivers/power/reset/sc27xx-poweroff.csc27xx-poweroff.c) | **Vendor feasible match** #41 (alias `sc27xx-poweroff` dari vendor Advan `sc27xx-poweroff.ko`, match dengan port 6.18) — Power `[V-FEASIBLE]` |
| 127 | `sc27xx-vibra` | [drivers/sc27xx-vibra.c](6.18.52-VXTux-UMS512/drivers/input/misc/sc27xx-vibra.csc27xx-vibra.c) | **Vendor feasible match** #42 (alias `sc27xx-vibra` dari vendor Advan `sc27xx-vibra.ko`, match dengan port 6.18) — Vibrator `[V-FEASIBLE]` |
| 128 | `rtc-sc27xx` | [drivers/rtc-sc27xx.c](6.18.52-VXTux-UMS512/drivers/rtc/rtc-sc27xx.crtc-sc27xx.c) | **Vendor feasible match** #40 (alias `rtc-sc27xx` dari vendor Advan `rtc-sc27xx.ko`, match dengan port 6.18) — RTC `[V-FEASIBLE]` |
| 129 | `pwm-sprd` | [drivers/pwm-sprd.c](6.18.52-VXTux-UMS512/drivers/pwm/pwm-sprd.cpwm-sprd.c) | **Vendor feasible match** #38 (alias `pwm-sprd` dari vendor Advan `pwm-sprd.ko`, match dengan port 6.18) — PWM `[V-FEASIBLE]` |
| 130 | `pinctrl-sprd` | [drivers/pinctrl-sprd.c](6.18.52-VXTux-UMS512/drivers/pinctrl/sprd/pinctrl-sprd.cpinctrl-sprd.c) | **Vendor feasible match** #36 (alias `pinctrl-sprd` dari vendor Advan `pinctrl-sprd.ko`, match dengan port 6.18) — Pinctrl `[V-FEASIBLE]` |
| 131 | `pinctrl-sprd-sharkl5Pro` | [drivers/pinctrl-sprd-sharkl5Pro.c](6.18.52-VXTux-UMS512/drivers/pinctrl/sprd/pinctrl-sprd-sharkl5pro.cpinctrl-sprd-sharkl5Pro.c) | **Vendor feasible match** #37 (alias `pinctrl-sprd-sharkl5Pro` dari vendor Advan `pinctrl-sprd-sharkl5Pro.ko`, match dengan port 6.18) — Pinctrl `[V-FEASIBLE]` |
| 132 | `phy-sprd-sharkl5Pro` | [drivers/phy-sprd-sharkl5Pro.c](6.18.52-VXTux-UMS512/drivers/phy/sprd/phy-sprd-ums512.cphy-sprd-sharkl5Pro.c) | **Vendor feasible match** #34 (alias `phy-sprd-sharkl5Pro` dari vendor Advan `phy-sprd-sharkl5Pro.ko`, match dengan port 6.18) — PHY `[V-FEASIBLE]` |
| 133 | `musb_sprd` | [drivers/musb_sprd.c](6.18.52-VXTux-UMS512/drivers/usb/musb/musb_sprd_vxtux.cmusb_sprd.c) | **Vendor feasible match** #32 (alias `musb_sprd` dari vendor Advan `musb_sprd.ko`, match dengan port 6.18) — USB `[V-FEASIBLE]` |
| 134 | `sprdbt_tty` | [drivers/sprdbt_tty.c](6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/bluetooth/sprdbt_tty.c) | **Vendor feasible match** #83 (alias `sprdbt_tty` dari vendor Advan `sprdbt_tty.ko`, match dengan port 6.18) — Bluetooth `[V-FEASIBLE]` |
| 135 | `sprdwl` | [drivers/sprdwl.c](6.18.52-VXTux-UMS512/drivers/net/wireless/sprd/sprdwcn/wlan/sprdwl.c) | **Vendor feasible match** #91 (alias `sprdwl` dari vendor Advan `sprdwl.ko`, match dengan port 6.18) — WiFi `[V-FEASIBLE]` |
| 136 | `sblock_bridge` | [drivers/sblock_bridge.c](6.18.52-VXTux-UMS512/drivers/soc/sprd/sipc/sblock_bridge.c) | **Vendor feasible match** #49 (alias `sblock_bridge` dari vendor Advan `sblock_bridge.ko`, match dengan port 6.18) — SIPC `[V-FEASIBLE]` |
| 137 | `sbuf_bridge` | [drivers/sbuf_bridge.c](6.18.52-VXTux-UMS512/drivers/soc/sprd/sipc/sbuf_bridge.c) | **Vendor feasible match** #50 (alias `sbuf_bridge` dari vendor Advan `sbuf_bridge.ko`, match dengan port 6.18) — SIPC `[V-FEASIBLE]` |
| 138 | `spipe` | [drivers/spipe.c](6.18.52-VXTux-UMS512/drivers/soc/sprd/sipc/spipe.c) | **Vendor feasible match** #70 (alias `spipe` dari vendor Advan `spipe.ko`, match dengan port 6.18) — SIPC `[V-FEASIBLE]` |
| 139 | `spool` | [drivers/spool.c](6.18.52-VXTux-UMS512/drivers/soc/sprd/sipc/spool.c) | **Vendor feasible match** #73 (alias `spool` dari vendor Advan `spool.ko`, match dengan port 6.18) — SIPC `[V-FEASIBLE]` |
| 140 | `seth` | [drivers/seth.c](6.18.52-VXTux-UMS512/drivers/soc/sprd/sipc/seth.cseth.c) | **Vendor feasible match** #47 (alias `seth` dari vendor Advan `seth.ko`, match dengan port 6.18) — SIPC `[V-FEASIBLE]` |
| 141 | `sipc-core` | [drivers/sipc-core.c](6.18.52-VXTux-UMS512/drivers/soc/sprd/sipc/sipc.csipc-core.c) | **Vendor feasible match** #60 (alias `sipc-core` dari vendor Advan `sipc-core.ko`, match dengan port 6.18) — SIPC core `[V-FEASIBLE]` |
| 142 | `sipx` | [drivers/sipx.c](6.18.52-VXTux-UMS512/drivers/soc/sprd/sipc/sipx.c) | **Vendor feasible match** #61 (alias `sipx` dari vendor Advan `sipx.ko`, match dengan port 6.18) — SIPC zero-copy `[V-FEASIBLE]` |
| 143 | `sprd-sipc-virt-bus` | [drivers/sprd-sipc-virt-bus.c](6.18.52-VXTux-UMS512/drivers/soc/sprd/sipc/sprd-sipc-virt-bus.c) | **Vendor feasible match** #111 (alias `sprd-sipc-virt-bus` dari vendor Advan `sprd-sipc-virt-bus.ko`, match dengan port 6.18) — SIPC virt-bus `[V-FEASIBLE]` |
| 144 | `leds-sc27xx-bltc` | [drivers/leds/leds-sc27xx-bltc.c](6.18.52-VXTux-UMS512/drivers/leds/leds-sc27xx-bltc.c) | **Vendor feasible match** #26 (alias `leds-sc27xx-bltc` dari vendor Advan `leds-sc27xx-bltc.ko`, match dengan port 6.18) — LED `[V-FEASIBLE]` |


> **Legenda & status verifikasi desu~:** "Tier A" = sumber + simbol + config terverifikasi ada di tree; "Tier B" = bukti simbol/reverse-engineering (radare2/ghidra); "Mainline" = kode upstream 6.18 apa adanya. **13 entri sudah LULUS kompilasi penuh di device** (gcc 16.2.1, artix proot — compiler asli kernel, exit 0, nol undefined symbol; 2026-10-15): `sprd_vsp.ko` 231 KB, `cpp.ko` 321 KB, `sprd-iommu.ko` 81 KB, `sprd_ion.ko` 62 KB, `sprd_camsys_pw_domain.ko` 95 KB, `sprd_vsp_pw_domain.ko` 52 KB, `sprd_battery_info.ko` 96 KB, `wcn_core.ko` (composite), `sprdwl.ko` 2,5 MB, `sprd_jpg.ko` 156 KB, **`sprd_vdsp.ko` 913.568 byte** (TICK7), **`sprd-gsp.ko` 767.488 byte** (TICK9), **`novatek_ts_spi.ko` 512.584 byte** (TICK9). **`sprd_flash_drv` + `spi-sprd` closed via System.map** (TICK9): 14 + 23 simbol build-in di `work/sysmaps/System.map.builddmed.20261014_2218` (md5 `82055ecc...`). **TICK10** (commit `e27dda4c1`): thermal DT chain 1/4 (`pmic_tsensor` di sc2730.dtsi + `osctsen/outtsen-thmzone` di vxtux.dts) + retro-fix `sprd_vdsp` wiring + donor 9.929 baris akhirnya masuk `.git`. Sisanya verified-in-tree (config + DT + sumber ada); bukti runtime (probe dmesg) menunggu boot kernel custom 6.18.52 di hardware — device sedang sidang.
>
> 📋 **40 driver relevan Advan T1030** dikategorikan per-fungsi di [`docs/DRIVERS_RELEVANT.md`](docs/DRIVERS_RELEVANT.md): A) Boot-Critical (8), B) Daily Functionality (14), C) SIPC/Modem (10), D) Sensor/PM/Thermal (11). Sumber bukti: `kolist.1012b.txt` (1.424 `.ko`), `System.map.builddmed.20261014_2218` md5 `82055ecc…`.
>
> 🔍 **Cross-check Vendor Advan 129 .ko** (dari `dumped/re/hidden/vramdisk_ko/`, kernel 5.4 stock): di tabel di atas, kolom **Vendor 129** = `[V]` artinya driver ada di vendor dump Advan (90/103 entry match), `[NV-X]` artinya tidak (13/103 — dengan kategori penyebab). Detail per-entry di [`docs/DRIVERS_VENDOR_CROSSCHECK.md`](docs/DRIVERS_VENDOR_CROSSCHECK.md).
>
> 📊 **Klasifikasi akar penyebab 13 NV entries** (di kolom Vendor 129, suffix `A/B/C/D/E`) — **diperbarui TICK11 (2026-10-15): 10/13 naik jadi [V] atau [V-ALT]; 3 NV-E tetap [NV-E] by design**:
> - **A — Driver baru (port dari mainline/DT, vendor engga punya)**: 6 entry. SEMUA diaktifkan di fragmen TICK7-TICK10 — `(sc27xx_tsensor_thermal L1204, sprd_cpu_cooling L1223, sprd_gpu_cooling L310, sprd_map L253, sprd_u_ether L659, sprd_usb_f_rndis L659)` — dan diinventaris ulang sebagai `[V]` (TICK11, 2026-10-15). Tidak ada lagi NV-A.
> - **C — Vendor composite dipecah jadi 3 di port**: 2 entry. `sprd-top-dvfs` (= alias of_match ke-1 `apsys-dvfs`, `[V-ALT] apsys-dvfs`) + `sprd_cp_dvfs` (= alias of_match ke-2, fix nama: `sprd_audcp_dvfs`, `[V]`). TICK11 inventaris ulang.
> - **D — Entry obsolete (sumber + vendor engga ada)**: 2 entry. `sprd_ddr_dvfs` (alias of_match ke-3 apsys-dvfs, `[V-ALT] apsys-dvfs`) + `sprd_pmic_wdt` (rename ke `sprd_wdt_fiq`, `[V] sprd_wdt_fiq`, `drivers/watchdog/sprd_wdt_fiq.c` + `CONFIG_SPRD_WATCHDOG_FIQ=y` di fragmen). TICK11 inventaris ulang.
> - **E — Mainline generic / BUILTIN, NV by design**: 3 entry. `thermal-generic-adc` (mainline), `panfrost` (mainline GPU, REPLACES vendor Mali Gondul), `sprd_thermal` (=y BUILTIN ke vmlinux). **TIDAK di-fix — by design**.

---

## 🎮 Graphics note | GPU メモ

GPU T618 itu Mali-G52 MP2, bukan G57. Stock ID `0x7402` dinormalisasi Panfrost jadi `0x7002`. DTS pake `arm,mali-bifrost`.

## 🛠️ Build | ビルド方法

**Host cross-compile** (config + DTB; kbuild host build mentok di header generated kbuild — `types/div64/cpucap-defs` — itu infrastruktur, bukan bug port):

```sh
cd 6.18.52-VXTux-UMS512
make O=out ARCH=arm64 defconfig
(
	cd out
	sh ../scripts/kconfig/merge_config.sh -m .config \
		../arch/arm64/configs/vxtux_618_fragment.defconfig
)
make O=out ARCH=arm64 olddefconfig
make O=out ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- Image modules dtbs
```

DTB: `out/arch/arm64/boot/dts/sprd/ums512-1h10-vxtux.dtb`

**Verifikasi ARM64 penuh = build di device** (Termux + proot-distro artix + gcc 16.2.1, compiler asli kernel stock):

```sh
# di device, dalam container artix
make O=buildd ARCH=arm64 CC=gcc syncconfig
make O=buildd ARCH=arm64 CC=gcc M=drivers/media/platform/sprd/vsp modules   # modul tunggal
make -j8 O=buildd ARCH=arm64 CC=gcc Image modules                            # kernel penuh
```

Catatan build device: (1) setiap edit `.config`/Kconfig wajib `syncconfig` dulu — `M=` build tidak refresh `auto.conf`; (2) simbol antar-modul di-merge dari `Module.symvers` lokal tiap dir ke `buildd/Module.symvers`; (3) file hasil sync host→device strip CRLF dulu (hapus karakter CR di akhir setiap baris); (4) setelah build gagal di sebuah dir, `rm -f *.o` sebelum rebuild (`.o` partial bisa dianggap up-to-date → modpost "truncated").

## 📊 Appendix A — 103 (port) × 129 (vendor) Cross-Check

Konsolidasi dari dua sumber data driver:
- **103 driver port** (inventory `README.md` atas): driver hasil porting/kernel 6.18.52-VXTux-UMS512
- **129 driver vendor** (dari `dumped/re/hidden/vramdisk_ko/`): driver board-specific Unisoc T618/UMS512 dari vendor_ramdisk_stock Advan T1030 (kernel 5.4 stock)

### A.1 Statistik cross-check

| Sumber | Total | Match | Tidak match | Catatan |
|---|---:|---:|---:|---|
| **Port 103** (kernel 6.18) | 103 | **90** (ada di vendor) | **13** (NV) | Dilihat dari perspektif port |
| **Vendor 129** (kernel 5.4) | 129 | **41** (ada di port) | **88** (vendor-only) | Dilihat dari perspektif vendor |
| **Gabungan unik** | ~150 | — | — | Ada overlap (misal `core.ko` di vendor = `wcn_core.ko` di port) |

**Penjelasan asimetri**:
- **90 V dari port** = driver yang dibuild/port dan ada di vendor Advan. (lebih besar dari 41 match vendor→port karena satu entry port bisa match banyak nama vendor, e.g. `wcn_bsp` + `wcn_core` di vendor match 1 entry port `core`).
- **41 V dari vendor** = driver vendor yang ada di port 103.
- **88 vendor-only** = driver yang dipakai Advan stock tapi BELUM di-port ke 6.18 (untuk kerja berikutnya).
- **13 NV dari port** = driver di port yang TIDAK ada di vendor (kategori A/B/D/E).

### A.2 Klasifikasi 13 NV entries (dari perspektif port)

| # | Ledger ID | Vendor match | Kategori | Sebab |
|---:|---|---|:---:|---|
| 45 | `sc27xx_tsensor_thermal` | NONE (vendor engga punya; TICK10 retro-fit DT) | **A** | Driver baru dari mainline/DT — ditulis TICK10 |
| 65 | `sprd-top-dvfs` | apsys-dvfs.ko (1 modul dipecah jadi 3 di port) | **C** | Vendor composite: apsys-dvfs.ko = top + cp + ddr |
| 73 | `sprd_cp_dvfs` | sprd_audcp_dvfs.ko (alias) | **C** | Vendor sprd_cp_dvfs = sprd_audcp_dvfs.ko. Entry ledger ambigu |
| 75 | `sprd_cpu_cooling` | NONE (mainline cpu_cooling generic) | **A** | Driver baru (late_initcall, tanpa of_match_table) |
| 76 | `sprd_ddr_dvfs` | apsys-dvfs.ko (alias dari top-dvfs) | **D** | Entry OBSOLETE: sumber engga ada, vendor engga punya. Seharusnya dihapus |
| 79 | `sprd_gpu_cooling` | NONE (mainline gpu_cooling generic) | **A** | Driver baru (CONFIG_SPRD_GPU_COOLING=y built-in) |
| 80 | `sprd_map` | NONE (vendor pakai ION/System.map) | **A** | Driver baru (sumber ION/System.map equivalent) |
| 83 | `sprd_pmic_wdt` | sprd_wdt_fiq.ko (nama beda) | **D** | Entry OBSOLETE: sumber engga ada. Seharusnya diganti sprd_wdt_fiq |
| 86 | `sprd_thermal` | NONE (BUILTIN =y) | **E** | BUILTIN ke vmlinux, engga jadi .ko |
| 87 | `sprd_u_ether` | NONE (vendor engga aktifkan USB gadget) | **A** | Driver baru (USB Ethernet gadget) |
| 88 | `sprd_usb_f_rndis` | NONE (vendor engga aktifkan USB gadget) | **A** | Driver baru (USB RNDIS gadget) |
| 93 | `thermal-generic-adc` | NONE (mainline) | **E** | Mainline generic, NV by design |
| 103 | `panfrost` | NONE (mainline GPU) | **E** | Mainline GPU driver, REPLACES vendor Mali Gondul DDK |

**Ringkasan kategori (13 NV)**:
- **A** (6) — Driver baru: `sc27xx_tsensor_thermal`, `sprd_cpu_cooling`, `sprd_gpu_cooling`, `sprd_map`, `sprd_u_ether`, `sprd_usb_f_rndis`
- **C** (2) — Composite pecahan: `sprd-top-dvfs`, `sprd_cp_dvfs` (vendor punya 1 `apsys-dvfs.ko`)
- **D** (2) — Entry obsolete: `sprd_ddr_dvfs`, `sprd_pmic_wdt` (sebaiknya dihapus atau diganti)
- **E** (3) — Mainline/BUILTIN: `thermal-generic-adc`, `panfrost`, `sprd_thermal`

### A.3 88 vendor-only (driver di vendor Advan, BELUM di-port)

Driver yang dipakai Advan T1030 stock (kernel 5.4) tapi belum ada di port kernel 6.18.52 kita. Ini **beban kerja berikutnya**:

**Kategori Clock/Regulator/GPIO** (5):
- `clk-sprd.ko` — clock controller SPRD vendor
- `ums512-clk.ko` — clock UMS512 (replacement `clk-sprd`)
- `sc2730-regulator.ko` — regulator SC2730
- `gpio-sprd.ko` + `gpio-eic-sprd.ko` + `gpio-pmic-eic-sprd.ko` — GPIO controller variants

**Kategori I2C/SPI** (4):
- `i2c-sprd.ko`, `i2c-sprd-hw.ko` — I2C bus + HW
- `spi-sprd-adi.ko` — SPI ADI variant
- `sdhci-sprd.ko` — SD card host

**Kategori MMC/DVFS** (3):
- `mmc_hsq.ko`, `mmc_swcq.ko`, `mmdvfs.ko`

**Kategori Efuse/NVMEM** (3):
- `nvmem_sprd_efuse.ko`, `nvmem_sprd_cache_efuse.ko`, `nvmem-sc27xx-efuse.ko` (VITAL untuk thermal chain TICK9 §4)

**Kategori SIPC/Modem** (5):
- `sipc-core.ko`, `sipx.ko`, `spipe.ko`, `spool.ko` (sudah ada di port!), `slog_bridge.ko`
- `sblock_bridge.ko`, `sbuf_bridge.ko` (sudah ada di port)

**Kategori Power/Misc** (10):
- `sprd_7sreset.ko`, `sprd_hwspinlock.ko`, `sprd_systimer.ko`, `sprd_shm.ko`, `sprd_power_stat.ko`
- `sprd_pdbg.ko`, `sprd_manufacturer_model.ko`
- `sprd_modem_loader.ko`, `sprd_sip_svc.ko`, `sprd-sipc-virt-bus.ko` (sudah ada di port)
- `sprd_iq.ko`, `sprd_soc_id.ko` (sudah ada di port)
- `sprd_time_sync.ko`, `sprd_time_sync_cp.ko` (sudah ada di port)
- `sprd_wdt_fiq.ko` (= `sprd_pmic_wdt` di ledger, sumber ada di tree)
- `sprd-cpufreq-public.ko`, `sprd-cpufreq-v2.ko`
- `sprd-sc27xx-spi.ko`

**Kategori Sensor** (1):
- `sensorhub.ko`

**Kategori Mailbox/USB** (2):
- `unisoc-mailbox.ko`
- `usb_f_vser.ko` (USB gadget virtual serial)

**Kategori Trusty TEE** (5): `trusty-ipc.ko`, `trusty-irq.ko`, `trusty-log.ko`, `trusty-pm.ko`, `trusty-virtio.ko` (5 dari 6 trusty vendor — di port cuma `trusty-tui.ko`)

**Kategori Mainline (di vendor tapi bukan vendor-specific)** (8):
- `apr.ko`, `act_gact.ko`, `act_gate.ko`, `act_mirred.ko`, `adc_keys.ko`
- `amlogic-gxl-crypto.ko`, `amphion-vpu.ko`, `analogix_dp.ko`, `adv7511.ko`, `mali_gondul.ko` (vendor Mali, REPLACED by panfrost)

**Kategori MMC** (3): `mmc_hsq.ko`, `mmc_swcq.ko`, `mmdvfs.ko`

### A.4 Sumber bukti & file

- **Port 103 inventory**: tabel utama README di atas (header `## 📋 Inventory per entri`)
- **Vendor 129**: `dumped/re/hidden/vramdisk_ko/` (129 .ko dari `vendor_ramdisk_stock.cpio` extraction)
- **Tally build TICK9**: `work/sysmaps/kolist.1012b.txt` (1.424 .ko, tapi majoriti mainline)
- **TICK10 commit `e27dda4c1`**: thermal DT chain 1/4 + sprd_vdsp retro-fix

### A.5 Action items (urut prioritas)

1. **Hapus/rename 2 entry obsolete (kategori D)**: `sprd_ddr_dvfs` + `sprd_pmic_wdt` (atau rename ke `sprd_wdt_fiq`).
2. **Port 88 vendor-only** yang vital:
   - **Efuse** (3 .ko) — wajib untuk thermal DT chain (TICK9 §4) TICK11+
   - **GPIO SPRD** (3 .ko) — untuk kontrol hardware
   - **Trusty TEE** (5 .ko) — untuk secure boot
   - **Mailbox** (1) — untuk AP↔CP IPC
3. **Re-run `make modules` TICK10+** untuk verifikasi vdsp wiring + thermal DT.
4. **Boot test** (satu-satunya cara verifikasi driver beneran hidup di hardware).

— Berry Chan, Raspya, 2026-10-15 🍓

## 📝 Changelog | 変更履歴

### 2026-10-15 — TICK10: thermal DT chain 1/4 + sprd_vdsp wiring fix
- **TICK9 §4 thermal DT chain — DIPROSES 1/4.** `pmic_tsensor: tsensor@1b34` (parent + `osc-tsensor@0` + `tout-tsensor@1`) ditulis di `sc2730.dtsi`; `osctsen-thmzone` + `outtsen-thmzone` thermal zones ditulis di `ums512-1h10-vxtux.dts` (sumber: DTB stok `fdt.dts:5869-5881`). Header `sc2730.dtsi` dikoreksi: komentar "tsensor belum punya driver mainline" USANG (driver `sc27xx_tsensor_thermal.ko` ter-build 2026-10-13). Bukti + retraksi 1 klaim: `docs/DEBLOB-20261015-TICK10-STATUS.md` §1.1. **Batas jujur**: node ini membuka jalan `sc27xx_tsensor` BISA bind; probe masih exit early kalau `/chosen/bootargs` tidak memuat `androidboot.mode=cali`. Sisa 3/4 (TICK11): `ap_efuse@32240000` + 13 sel kalibrasi + 3 `ap_thm0/1/2` + `virtual_sensor@0` + 10 zona thermal.
- **TICK7 retro-fix sprd_vdsp akhirnya di-commit.** TICK7 commit `4152b927f` mengaku vdsp "ported" — faktanya file + wiring TIDAK pernah di-`git add` sampai tick ini. Yang masuk `.git`: `drivers/vxtux/npu/vdsp/{Kconfig,Makefile,ion.h}` (90 baris) + `common/{sprd_vdsp_mm.h,vdsp_trusty.h}` (240 baris) + `Cadence/xrp/*.{c,h}` (7.860 baris .c + 14 .h, donor catalog `mods-sprd/sprd/kernel4.14/npu/vdsp` Cadence/XRP) + `drivers/devfreq/sprd/apsys/{sprd_dvfs_vdsp.h,vdsp_dvfs_sharkl5pro.h}` (186 baris, donor iscle W20.13.3) + wiring `drivers/vxtux/{Kconfig,Makefile}` + `CONFIG_SPRD_VDSP=m` di fragmen + `sprd_iommu_*_with_idx` exports di `drivers/iommu/sprd-iommu.c`. Fragmen juga dibersihkan: `CONFIG_VIDEO_GC5025=y` duplikat + blok `VXTUX-BRINGUP-BEGIN` 69 baris (CONFIG_BT/CFG80211/MEDIA_SUPPORT/VIDEO_GC8034) dihapus (duplikat dengan baris lain). Bukti + catatan ion shim + dvfs notifier stub: `docs/DEBLOB-20261015-TICK10-STATUS.md` §1.2. **Batas jujur**: vdsp BELUM di-build di device tick ini; `make M=drivers/vxtux/npu/vdsp` menunggu device sidang.
- **Push `test1` ke origin `archanaberry/advan-vxtux-6.18.52`** (TICK10): branch dari main lokal (`69acfaacf`) + TICK8 `4152b927f` + TICK9 `e7cef58d3` + TICK9 addendum `a4181022a` + TICK10 (commit baru) — 5+ commit ahead of `origin/main`. PR-ready: https://github.com/archanaberry/advan-vxtux-6.18.52/pull/new/test1
- **Sisa gap jujur TICK10 → TICK11**: (1) verifikasi runtime `insmod` 13 modul di hardware — device sedang sidang; (2) resync 1.435 `.ko` ke config de-bloat — wajib sebelum insmod/flash; (3) Image belum di-flash/boot-test; (4) thermal DT chain 3/4 sisanya; (5) **vdsp build verifikasi** di device (TICK7 angka 913.568 byte dari config pra-debloat mtime 09:40 WIB, bukan konfigurasi sekarang).

### 2026-10-15 — TICK9: ledger 102/102 DEBLOBBED, 4 baris terakhir ditutup
- **Ledger 102/102 DEBLOBBED** (sebelumnya 98/3/1). Empat baris terakhir ditutup dengan bukti terukur hari ini: `sprd_flash_drv` + `spi-sprd` (System.map segar 14 Okt 22:18 WIB, md5 `82055ecc...` — 14 + 23 simbol build-in: `T sprd_flash_ctrl`, `t sprd_flash_probe`, `T sprd_spi_probe`, `d sprd_spi_of_match`, `t sprd_spi_driver_init`, dll); `sprd-gsp` + `novatek_ts_spi` (`.ko` build penuh: `sprd-gsp.ko` 767.488 B md5 `bfefe9af...` dengan llvm-nm **535 simbol terdefinisi** / 111 undefined; `novatek_ts_spi.ko` 512.584 B md5 `943c7d89...` dengan llvm-nm **337 simbol terdefinisi** / 88 undefined). Bukti + retraksi 5 klaim usang: `docs/DEBLOB-20261015-TICK9-STATUS.md`.
- **13 modul compile-verified device** (was 10 TICK8 → 13 TICK9). sprd-gsp claims tick-13 "0 dari 45 fungsi gsp_dev.c terhubung" DICABUT — faktanya 535 simbol terhubung.
- **System.map segar diarsipkan**: `work/sysmaps/System.map.builddmed.20261014_2218` (md5 `82055ecc...`, 9.458.636 B); `.config` segar `work/sysmaps/dotconfig.builddmed.20261014_2218` (md5 `f9f1080f...`); `work/sysmaps/sprd-gsp.ko.20261014_0940` (md5 `bfefe9af...`); `work/sysmaps/novatek_ts_spi.ko.20261014_0940` (md5 `943c7d89...`).
- **DTB `touchscreen@0` (novatek,NVT-ts-spi)** terverifikasi ada di DTB hasil build (dtc decompile baris 1566-1577) cocok dengan `nvt_match_table` (nt36xxx.c:2407). Update dari catatan TICK8: ada node DT thermal (`pmic_tsensor`, `osctsen/outtsen-thmzone` — baris 1199, 2269-2278, phandle 0x72).
- **Catatan jujur**: (a) kedua `.ko` dibangun dengan config **pra-debloat** (mtime 09:40 WIB) — config segar (22:18) masih `=m` untuk keduanya, jadi status baris tidak berubah, hanya mtime .ko yang basi. Resync `make modules` pasca-debloat masih terbuka. (b) GSP `struct gsp_cfg` direkonstruksi dari offset blob — kebenaran layout TIDAK terbukti, kalau salah driver bisa probe lalu korup layer. (c) `sprd_cpu_cooling` memang tidak butuh DT (late_initcall, tanpa of_match_table) — model vendor, bukan cacat.
- **Thermal DT chain belum lengkap** (peta TICK9 §4): `ap_efuse` (efuse@32240000) + ~13 sel kalibrasi + 3 `ap_thm0/1/2` (thermal@32200000/32210000/32220000) + `virtual_sensor` node belum ada. `sprd_thermal` BUILTIN + `sprd_soc_thm.ko` baru akan bind kalau itu lengkap. Setengah jalan akan menghasilkan node yang tidak pernah bind (pola `sprd_camsys_pw_domain`).

### 2026-10-14 — TICK8: de-bloat Image + DTB baru + sprd_vdsp ported (bestie~)
- **`sprd_vdsp` ported desu!** — 31 file donor 4.14 `iscle_ums512` (Cadence XRP) → `drivers/vxtux/npu/vdsp/`. Total `sprd_vdsp.ko` **913.568 byte** (relink penuh, log `t21_full.log` 10:24 UTC). 10 entri compile-verified sekarang (sebelumnya 6). Sisa 93 = verified-in-tree. Fix build dirangkum di #89 (donor elf.h bentrok uapi/linux/elf.h → 22 typedef error; ABI 4.14→6.18; shim ion; iommu tanpa `_with_idx`).
- **Image de-bloat 59 → 50,5 MB (−14,5%)** — penyebab: +40 opsi `CONFIG_DEBUG_*` / `TRACING` (LOCKDEP, PROVE_LOCKING, DEBUG_OBJECTS×6, DEBUG_VM, TRACING, EVENT_TRACING, …) yang masuk dari restore config sesi t28. Dimatikan via `work/pull/debug_options_to_disable.txt` (43 opsi). Sisanya ≈3,3 MB selisih vs baseline 47,2 MB = 118 driver `=m→=y` yang sengaja dipertahankan (fitur, bukan kebocoran — lihat SPI_SPRD=y & SPRD_FLASH_DRV=y). Banner `Linux version 6.18.52-VXTux-UMS512` cocok dua kali.
- **DTB diperbarui 7 Okt → sekarang** — `ums512-1h10-vxtux.dtb` **45.702 byte** (dari 32.558), 17 detik build target tunggal. 5 file dts di-sync host→device (backup `.bak.dtsync20261014`), termasuk `ums512-modem-2h10.dtsi` yang belum ada di device. Node kunci: `sprd-marlin3`, `bq256xx-charger`, `cm-chargers`, `vddcama0`, `modem@1`, `vbc-iis3-0`, `mali-bifrost`, `battery×20` — semua terdecompile ✓.
- **Push `test1` ke origin `archanaberry/advan-vxtux-6.18.52`** — branch dari main lokal (`69acfaacf` "tally 102/103"), 5 commit ahead of `origin/main` (yang masih 19b381309 "tally 9-10"). Bestie bisa review dulu sebelum merge ke main. PR-ready: https://github.com/archanaberry/advan-vxtux-6.18.52/pull/new/test1
- **Sisa gap jujur**: (1) verifikasi runtime `insmod` sprd_vdsp.ko — device sedang sidang, hanya kompilasi+link yang terbukti; (2) **2.871 .ko belum di-resync** ke config de-bloat (mereka dari config berdebug) — wajib sebelum insmod/flash; (3) Image belum di-flash/boot-test; (4) DTB 17 detik itu target tunggal, dtbs penuh 1.544 file belum dibangun ulang (memory penuh di sesi sebelumnya, butuh OOM=off atau j≤2).
- **Dokumentasi sinkron**: `docs/HANDOFF-20261014-BUFFY-TO-HERMES.md` (13,5 KB) + salinan identik di `C:\Users\Administrator\AppData\Local\hermes\HANDOFF-BUFFY-20261014.md`. 5 bagian: peringatan koordinasi antar-sesi, status kernel 103/103, resep build chroot (rootfs artix + 6 bind mount + `su 0 -mm` + shim bc), prosedur push ke Archana lewat fork, jebakan lingkungan. (Note: sesi ini pakai SSH key langsung `id_ed25519_repo` untuk archanaberry101 — bukan fork, karena user `archanaberry` adalah akun bestie sendiri.)

### 2026-10-10 — perbaikan wiring + donor baru (build device belum dijalankan)
- **`sprd-gsp` = 1 modul komposit.** `CONFIG_DRM_SPRD_GSP=m` + sembilan objek di satu baris `obj-` = sembilan modul terpisah; `gsp_core.c` tidak mengekspor satu pun simbol `gsp_core_*` (nol `EXPORT_SYMBOL` di direktori itu) sehingga modpost gagal undefined dan minta `MODULE_LICENSE` per objek. Donor 5.4 (`fork_t618`) memakai `obj-y` sehingga tidak pernah menyentuh batas modul. Kini `sprd-gsp-objs` (nama modul = nama blob vendor) dengan objek subdirektori ditulis lewat path relatif; `gsp_interface/Makefile` + `gsp_lite_r2p0/Makefile` tidak lagi di-descend. `sprd_drm_gsp.h` juga dijaga `CONFIG_DRM_SPRD_GSP_MODULE` (tristate `=m` mendefinisikan ejaan itu, bukan `CONFIG_DRM_SPRD_GSP`) supaya stub inline tidak bentrok dengan badan aslinya. 26 baris `MODULE_LICENSE("GPL");` yang sebelumnya tertanam **di dalam blok komentar lisensi** (no-op) dibuang — lisensi cukup satu, di `gsp_dev.c`.
- **Donor baru terpakai: `bocchi_unisoc54` `drivers/unisoc_platform/debug_log/`** (via `work/donors_20260930/`). Yang diambil hanya trio inti; varian `sharkl5/` sengaja belum karena butuh register DBG_APB/MM_AHB_GEN_CLK_CFG yang headernya belum ada. Ikut masuk `include/linux/soc/sprd/sprd_sysdump.h` — berkas itu diminta `drivers/soc/sprd/modem/cp_dump/sprd_cp_dump.c`, jadi sebelum ini driver itu tidak bisa dikompilasi.
- **Empat stopper build lain** (kelas “gagal senyap”): `drivers/sprd/flash/Kconfig` `MISC_DEVICE` (simbol tidak ada → driver mustahil aktif) → `HAS_IOMEM`; `drivers/soc/sprd/modem/Kconfig` `FIRMWARE_LOADER` → `FW_LOADER`; `sprd_cproc.c` `early_param()` (tidak ada di jalur modul 6.18) → `module_param_named(modem, ...)`; `cpp/` + `sprd_sensor/` Makefile dapat `-I$(srctree)/include/uapi` untuk `<video/*.h>`.
- **Batas jujur:** semua di atas = review statis (host ini tanpa toolchain ARM64, build selalu di device). Perlu satu build device untuk menaikkan tally compile-verified.

### 2026-10-09 — tree restore + gap closing
- **Tree restored**: commit deblob sempat over-prune 37.103 file (semua DTS, drivers/gpu, dma-buf, iio, media, iommu, clk, crypto, block, defconfig). Dipulihkan penuh dari branch codespace + `.config` port (`dac18ecfa`). Nol file hilang permanen.
- **`.config` lengkap**: merge `vxtux_618_fragment.defconfig` resmi — membawa TRUSTY family, SPRD_ION/JPG/SENSOR/DMA, USB gadget, MUSB_HDRC, CHARGER_BQ256XX, TOUCHSCREEN_NOVATEK_NVT_TS, SPRD_AUDIO_PIPE, DMABUF_HEAPS_CMA.
- **Gap USB ditutup**: `CONFIG_USB_MUSB_SPRD_VXTUX=y` ditambahkan ke fragment — tanpa baris itu `musb_sprd_vxtux.o` tak pernah ter-compile padahal node DT `sprd,sharkl5pro-musb` ada (USB mati total). Bukti: Kconfig:204, Makefile:49, of_match:1568, DTS:676.
- **Camera graph diperbaiki**: port@1/port@2 dcam (dua master pada endpoint csi yang sama → "not bidirectional") dihapus. Pipeline: sensor → CSI → ISP → DCAM. DTB rebuild: 0 error, 5 warning kosmetik.
- **DTB rebuilt**: `ums512-1h10-vxtux.dtb` (45.314B, setelah node vsp+cpp) dari DTS terkoreksi — node panfrost `arm,mali-bifrost`, jpg-codec, isp/csi, novatek+himax touch, trusty, iommu, video-codec@20500000, cpp@62800000 semua terverifikasi via strings.
- **VSP codec diporting (2026-10-09)**: `sprd_vsp_main.c` (rename dari `sprd_vsp.c`)+`vsp_common.c`+`vsp_isr_func.c` dari donor iscle_ums512 5.4 (sama SoC) → `drivers/media/platform/sprd/vsp/` (Kconfig+Makefile+wire). `sprd_dvfs_vsp.h` dari archive v3 → `drivers/devfreq/apsys/`. Integrasi: `regs[]` TIDAK diduplikasi (pemilik tunggal `vsp_regs.c`, +EXPORT_SYMBOL), Kconfig `select SPRD_VSP_PW_DOMAIN`. Node DT `video-codec@20500000` ("sprd,sharkl5pro-vsp") nilai register dari stock fdt_live.dtb (reset/force/auto/domain_eb) + tabel debug vendor (pwr_status: PMU 0xBC mask 0x700); clock dikonfirmasi dt-bindings (CLK_VSP_EB=2, CLK_VSP=22). DTB rebuild: 0 error, node terverifikasi strings. TANPA power-domains (pw-domain library murni → defer trap) & TANPA iommus (compatible stock tak match driver tree; driver toleran). **Kompilasi ARM64 penuh: BUILD DEVICE SUKSES (2026-10-09)** — lihat entri build di bawah; 7 modul jadi .ko dengan gcc 16.2.1 (compiler asli kernel).
- **VSP+CPP+iommu build device LULUS (2026-10-09)**: semua .ko terbangun di device (artix proot + gcc 16.2.1) — `sprd_vsp.ko` 231 KB, `cpp.ko` 321 KB, `sprd-iommu.ko`, `sprd_ion.ko`, `sprd_camsys_pw_domain.ko`, `sprd_vsp_pw_domain.ko`, `vsp_regs.ko` (exit 0, nol undefined symbol). Bug yang ketemu & diperbaiki di run ini: (a) Makefile vsp 3 obj- terpisah → 3 modul terpisah, simbol internal undefined → jadi composite module `sprd_vsp-objs` (file utama rename `sprd_vsp_main.c`, pola zram); (b) `sprd_iommu_restore` dideklarasi header tapi tak pernah didefinisikan/di-export → ditambah + EXPORT_SYMBOL; (c) `compat_alloc_user_space()` tak ada di ARM64/6.18 → compat ioctl vsp di-rewrite pakai buffer kernel; (d) `sprd_iommu_resume/suspend` tak ada di pohon → resume→restore, suspend di-drop; (e) `CONFIG_SPRD_ION` tak terdeklarasi (olddefconfig drop) → dideklarasi; (f) struct `ion_phy_data` hilang (over-prune) → direstore; (g) `struct iommu_map_data`/`vsp_iommu_map_data` + MODULE_LICENSE/IMPORT_NS + ccflags-y -I fix. Sisa verifikasi runtime: probe di hardware (modul insmod + log dmesg) — belum dijalankan.
- **Kernel penuh build device LULUS (2026-10-09, 13:29)**: `make -j4 Image modules` **exit 0** — 12.516 langkah, 0 error, **Image baru 47,2 MB** (bukan lagi stale Sept 27), vmlinux 185 MB, **1.421 .ko** (16 vendor sprd: vsp/cpp/ion/iommu/3 pw-domain/drm/phy/wcn/bt/fm/power_manager/2 panel + 84 objek vendor =y masuk vmlinux). Catatan: percobaan pertama `-j8` GAGAL dengan "No such file" pada file intermediate — diagnosis: race proot (ptrace path-redirection drop syscall saat paralel berat), BUKAN disk/ownership; aturan baru: full build di proot pakai -j4. Artinya: **semua driver yang enabled di config sudah lolos compile** (bukan cuma 6 modul); sisa entry yang `=n` memang sengaja (misal `sprd_battery_info`). Sisa gap runtime: boot Image+DTB → insmod → dmesg.
- **CPP diporting (2026-10-09)**: 13 file vendor lengkap (`work/donorcatalog/mods-sprd/sprd/common/camera/cpp/lite_r6p0/` — cpp_core/cpp_ioctl/scale_drv/rot_drv/dma_drv + sprd_cpp.h/sprd_img.h interface) → `drivers/media/platform/sprd/cpp/` (Kconfig `select SPRD_CAMSYS_PW_DOMAIN` karena cpp_core.c:647/424 memanggil `sprd_cam_pw_on/off`). Node DT `cpp@62800000`: register dari stock fdt_live.dtb; clock `CLK_MM_CPP_EB=0`/`CLK_CPP=5` dikonfirmasi dt-bindings; `sprd,cam-ahb-syscon = <&mm_ahb_regs>` untuk MM_AHB_RESET (0x04, reset bits identik stock). TANPA power-domains/iommus/cpp_qos (driver tak butuh; lihat komentar node). DTB rebuild: 0 error, node terverifikasi strings. Tally: **103/103 driver di pohon** — gap vsp + cpp sama-sama tertutup; sisa = bukti runtime (probe logs) di hardware.
- **Sisa gap jujur**: TIDAK ADA lagi driver yang belum port — 103/103 verified-in-tree. Yang tersisa hanya bukti runtime di hardware (probe logs) untuk driver yang config+DT-nya lengkap; kompilasi ARM64 penuh dilakukan build device (cron loop), host cross-compile mentok di header generated kbuild.

---

## 📚 Referensi lain (belum dipakai) | Other references | その他リファレンス

Repo relevan UMS512/T618 (sharkl5pro) yang sudah dicari tapi BELUM kepakai di port — disimpan untuk pembanding/pekerjaan berikutnya. Sama 3 header, tanpa hitungan~

| Nama Repo | Sumber Repo (link) | Keterangan / Status |
| --- | --- | --- |
| torvalds/linux (mainline) | [git.kernel.org/torvalds/linux](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git) | Mainline punya `arch/arm64/boot/dts/sprd/ums512-1h10.dts` + `ums512.dtsi` — baseline DT UMS512 resmi. Pembanding struktur node upstream (tree kita = 6.18.52 + port vendor, bukan mainline murni). |
| beebono/u-boot-ums512 | [github.com/beebono/u-boot-ums512](https://github.com/beebono/u-boot-ums512) | U-Boot vendor sharkl5pro/UMS512 (board `ums512_1h10`). Referensi bootloader untuk pekerjaan U-Boot di lindroid4 — belum dipakai di kernel port. |
| QiTianCatKiss/ums512-device-tree | [github.com/QiTianCatKiss/ums512-device-tree](https://github.com/QiTianCatKiss/ums512-device-tree) | Ekstraksi + analisis device tree platform UMS512. Kandidat pembanding struktur node/register DT. |
| Seriousattempts/lineage_ums512_1h10 | [github.com/Seriousattempts/lineage_ums512_1h10](https://github.com/Seriousattempts/lineage_ums512_1h10) | Device tree LineageOS Retroid Pocket 3+ — **board `ums512_1h10`, T618, Mali-G52 MC2**: keluarga board sama persis dengan kita, vendor lineage beda. |
| TheGammaSqueeze/UnisocBypass | [github.com/TheGammaSqueeze/UnisocBypass](https://github.com/TheGammaSqueeze/UnisocBypass) | RE secure-boot Unisoc: patch SPL RSA-verify + uboot unlock, **terverifikasi di UMS512/T618 (RG Vita)** + UMS9620/T820. Sangat relevan untuk boot/unlock lindroid4. |
| Hadenix/android_kernel_alldocube_ums512 | [github.com/Hadenix/android_kernel_alldocube_ums512](https://github.com/Hadenix/android_kernel_alldocube_ums512) | Kernel Alldocube iPlay 40 (T1020S, board `ums512_1h10` — sama dengan kita). Vendor lineage kedua untuk pembanding driver. |
| allex2/android_device_alldocube_T1020S | [github.com/allex2/android_device_alldocube_T1020S](https://github.com/allex2/android_device_alldocube_T1020S) | TWRP device tree Alldocube iPlay 40 (ums512_1h10). Peta partisi/perangkat vendor kedua. |
| Veynamer/mainline-unisoc | [github.com/Veynamer/mainline-unisoc](https://github.com/Veynamer/mainline-unisoc) | Upstream mainline Unisoc (vendor branch aktif) — pembanding DT/driver versi mainline terbaru. |
| dumps.tadiphone.dev | [dumps.tadiphone.dev](https://dumps.tadiphone.dev/) | Stock dump ROM board `ums512_1h10` (firmware/vendor, GitLab). Sumber firmware pembanding selain dump device sendiri. |
| strongtz/linux-sprd | [github.com/strongtz/linux-sprd](https://github.com/strongtz/linux-sprd) | Linux 4.14 keluarga Unisoc T-series (T7520/T7510/T710/T510/T310…), dengan `sprd_sharkl5Pro_defconfig`; dipakai komunitas untuk iPlay40 (ums512_1h10). **Juga punya `arch/arm64/boot/dts/sprd/ums512-modem.dtsi`** (node modem DT UMS512) — ditemukan lewat pencarian modem/LTE/GPS/BT 2026-10-09, belum dipakai di port. |
| Kyros70/android_kernel_ums9230 | [github.com/Kyros70/android_kernel_ums9230](https://github.com/Kyros70/android_kernel_ums9230) | Kernel UMS9230 — SoC penerus keluarga T-series. Pembanding arsitektur generasi baru. |
| Kiciuk/unisoc-5.4 | [github.com/Kiciuk/unisoc-5.4](https://github.com/Kiciuk/unisoc-5.4) | Fork alternatif realme C33 (MocorDroid-S 5.4). |
| maxsteeel/kernel_unisoc-5.4 | [github.com/maxsteeel/kernel_unisoc-5.4](https://github.com/maxsteeel/kernel_unisoc-5.4) | Fork realme C53 (Infinix Smart 8). |
| beebono/linux-6-16-sprd | [github.com/beebono/linux-6-16-sprd](https://github.com/beebono/linux-6-16-sprd) | Fork 6.16 ums512 (pensiun dari superproject rg-rotate setelah 7.1 parity, masih ada untuk referensi). |

### 🔬 Donor yang **KEMIRIPAN** (tidak dipakai, tapi struktur sama untuk cross-reference)

Repo dengan board/SoC keluarga yang **mirip** dengan UMS512/T618 tapi **TIDAK dipakai** untuk port. Disimpan untuk pembanding arsitektur + identifikasi driver. Urut dari yang paling mirip:

| Nama Repo | Sumber Repo (link) | Keterangan / Kesamaan |
|---|---|---|
| Kyros70/android_kernel_ums9230 | [github.com/Kyros70/android_kernel_ums9230](https://github.com/Kyros70/android_kernel_ums9230) | **SoC penerus** keluarga T-series. Arsitektur register/dvfs shared. Pembanding generasi baru. **(0 lokal, 0 dipakai)** |
| orangepi-xunlong/linux-orangepi | [github.com/orangepi-xunlong/linux-orangepi](https://github.com/orangepi-xunlong/linux-orangepi) | `uwe5622 unisocwifi` referensi WLAN lintas-platform (generasi lama, bukan drop-in `wlan_combo`). Mirip struktur WLAN driver. **(0 lokal, 0 dipakai)** |
| allex2/android_device_alldocube_T1020S | [github.com/allex2/android_device_alldocube_T1020S](https://github.com/allex2/android_device_alldocube_T1020S) | TWRP device tree Alldocube iPlay 40 (ums512_1h10). Peta partisi/perangkat vendor kedua. **(0 lokal, 0 dipakai)** |
| Kyros70/android_kernel_ums9230 | [github.com/Kyros70/android_kernel_ums9230](https://github.com/Kyros70/android_kernel_ums9230) | Tseries SoC penerus (ums9230). Driver framework shared dengan UMS512. **(0 lokal, 0 dipakai)** |
| realme-kernel-opensource/realme_C31_C33_C35_narzo50A-Prime | [realme-kernel-opensource/realme_C31_C33_C35_narzo50A-Prime-AndroidT-kernel-source](https://github.com/realme-kernel-opensource/realme_C31_C33_C35_narzo50A-Prime-AndroidT-kernel-source) | Fork MocorDroid-S kanonik — **induk keluarga iscle/bocchi/marohinmark** (angka di atas overlap: file yang sama bisa match beberapa fork). **(0 lokal, 0 dipakai)** |
| Seriousattempts/rp3plus-native-attempts | [github.com/Seriousattempts/rp3plus-native-attempts](https://github.com/Seriousattempts/rp3plus-native-attempts) | Belum dipakai — nol copy lokal, nol kutipan di docs. **(0 lokal, 0 dipakai)** |

### 📊 Repo yang dipakai vs tidak dipakai — TOTAL REKAP

| Kategori | Count | Keterangan |
|---|---:|---|
| **Dipakai** untuk port VXTux | **12** | `cdpkp` 67, `iscle` 55, `bocchi` 58, `marohinmark` 58, `Hadenix/wcn` 21, `sprd-oss-devs/ums512_bt_fm` 12, `Himax/HX83112` 5, `twrpdtgen` 4, `beebono/rg-rotate` 3, `MotorolaMobilityLLC/sprd` 2 |
| **Tersedia** (ada di fork) tapi tidak dikloning | 2 | `realme-kernel-opensource` 0 lokal, `orangepi-xunlong` 0 lokal |
| **Mirip arsitektur** (UMS9230 / T-series) | 2 | `Kyros70/ums9230` (penerus T-series), `allex2/T1020S` (board sama) |
| **Belum relevan** (RCU, mirror) | 1 | `Seriousattempts/rp3plus` |
| **TOTAL repo dicari** | **17** | — |
| **Total file vendor verbatim** | **298** (67+55+58+58+21+12+5+4+3+2 + 13 donor lain) | angka overlap: file yang sama bisa match beberapa fork |

> 💡 Yang paling worth dibuka berikutnya: **lineage_ums512_1h10** (board identik), **UnisocBypass** (secure-boot RE T618 terverifikasi), dan **u-boot-ums512** (bootloader).

## 🔧 Status | 状態

Active bring-up desu~ **TICK10 — thermal DT chain 1/4 + sprd_vdsp wiring fix, 103/103 driver punya artefak build; 13 modul lulus kompilasi penuh di device** (gcc 16.2.1, artix proot, exit 0, nol undefined symbol — 2026-10-15). Image `Linux version 6.18.52-VXTux-UMS512` 50,5 MB; DTB `ums512-1h10-vxtux.dtb` 45.702 byte; System.map `work/sysmaps/System.map.builddmed.20261014_2218` (md5 `82055ecc...`). TICK10 = (a) `pmic_tsensor` + 2 zona OSCT/OUTT ditulis di DTS, (b) sprd_vdsp wiring + donor 9.929 baris akhirnya masuk `.git`. Modul yang lulus build BUKAN bukti driver valid di hardware fisik — sisa verifikasi runtime: boot kernel custom 6.18.52 (Image + DTB) di device, lalu probe dmesg + device node (`/dev/sprd_vsp`, `/dev/sprd_cpp`, `/dev/sprd_vdsp`, `/dev/sprd-gsp`, `/dev/sprdbt_tty`) yaa~ (｡•̀ᴗ-)✧ Branch `test1` sudah ter-push ke origin `archanaberry/advan-vxtux-6.18.52` — bestie tinggal review & merge! Ledger deblob **102/102 closed** (TICK9); thermal DT chain sisanya (ap_efuse + 13 sel kalibrasi + 3 ap_thm + virtual_sensor) → TICK11.

<div align="center">

**VXTux-chan says: 一歩ずつ、確実に。TICK10 で thermal chain 1/4 まで漕ぎ着けたの！次は ap_efuse + sel kalibrasi だね〜 (๑•̀ㅂ•́)و✧**

</div>
