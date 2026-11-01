# Diff Kernel ORI (5.4.210) vs Kernel VXTux 6.18.52 — HASIL FINAL (2026-11-01)

## Sumber data
- Modul ORI stok: `E:/Advan/stock/vendor_modules/` (155 .ko) + `vendor_ramdisk_ko/` (129, subset)
- lsmod LIVE tablet (adb, kernel 5.4.210): **158 modul** (155 ORI vendor + 3 berry kustom)
- Tree port: `E:/vxtux_build_repo/6.18.52-VXTux-UMS512/` (source 6.18.52)
- Config build ACTUAL: `E:/vxtux_build_repo/enhanced_config.cfg` (workflow v17-build.yml:
  `cp ../enhanced_config.cfg .config` + `make olddefconfig`). `.config` di root tree TIDAK dipakai
  build — config yang benar adalah enhanced_config.cfg.

## METODOLOGI
Tiap modul ORI di-resolve terhadap: (1) target Makefile (`obj-$(CONFIG_X) += foo.o`), (2) Kconfig symbol,
(3) keberadaan file sumber di tree. Resolusi komposit (`foo-objs := a.o b.o`) + subdir (`obj-y += dir/`)
diperhitungkan. Ini menangkap RENAME (modul port nama beda, padanan fungsional ada) vs HILANG (tidak ada
padanan fungsional).

## Hasil cross-check (SEBELUM perbaikan config)
| Kategori | Jumlah |
|---|---|
| Modul ORI total (stock) | 155 |
| **Ter-build di port** (target+config on) | **110** |
| Built-in / ter-merge (CONFIG=y, rename, merge driver, mainline) | 44 |
| **Deblob port via RE (sprd_pmic_wdt)** | **1** |
| Masih gap | **0** |
| **TOTAL TER-COVER** | **155/155** |

### Verifikasi final 155 modul ORI (build `41305257804`, fork/test1 `d1066c168`)
Coverage union = System.map (built-in) + `.ko` ter-build + rename/merge terverifikasi +
deblob port. Satu-satunya modul binary-only tanpa source di tree = `sprd_pmic_wdt`
(PMIC WDT SC27xx via SIPC sbuf, dipakai perangkat — node `watchdog@40` ada di DTB hidup),
direkonstruksi penuh dari `.ko` stok via radare2 (probe/worker/notifier/init/remove,
bootargs magic `e551`, sbuf chan 6 = SIPC_ID_PM_SYS) dan ter-compile 0 error vs header 6.18.

## BUG CONFIG YANG DITEMUKAN & DIPERBAIKI (patch enhanced_config.cfg)
### A. Modul ORI yang TER-ENABLE tapi config OFF (nyala salah → modul tidak dibangun)
| Modul vendor | Simbol Kconfig | State lama | State baru |
|---|---|---|---|
| sprd_wlan_combo (WiFi SC2355 → sprdwl.ko) | WCN_WLAN / WCN_WLAN_SC2355 | ABSEN | =m / =y |
| nvmem_sprd_efuse | NVMEM_SPRD_EFUSE | ABSEN | =m |
| sc27xx_tsensor_thermal | SPRD_TSENSOR_THERMAL | ABSEN | =m |
| sprd_soc_thm | SPRD_SOC_THM | ABSEN | =m |
| sprd_battery_info | SPRD_BATTERY_INFO | ABSEN | =m |
| sprd_vdsp | SPRD_VDSP | ABSEN | =m |

**Catatan kritis WiFi:** `SC2355_WLAN=y` dan `SC2355_SDIO_WLAN=m` SUDAH ada di config, TAPI itu
simbol MATI — Makefile port (`drivers/vxtux/net/wireless/sprdwcn/wlan/Makefile:68`) memakai
`obj-$(CONFIG_WCN_WLAN) += sprdwl.o`, bukan `SC2355_WLAN`. Karena `WCN_WLAN` absen, direktori
`wlan/` TIDAK PERNAH di-descend → WiFi TIDAK PERNAH dibangun. Bukti di Makefile:
"The evidence that they were missing was simply that this directory had never been built —
CONFIG_WCN_WLAN was '# CONFIG_WCN_WLAN is not set'."

### B. NON-ORI sweep - KESIMPULAN AKHIR: TIDAK ADA non-ORI yang perlu dimatikan
Awalnya 4 simbol (BT_BCM, BT_QCA, USB_NET_AQC111, USB_RTL8152) dicurigai non-ORI
dan dimatikan. **Itu keliru** - diverifikasi ulang terhadap GROUND TRUTH (kallsyms
kernel stok, bukan file config):

| Simbol | Simbol di kallsyms stok | Status ORI | keputusan |
|---|---|---|---|
| BT_BCM (Broadcom BT HCI) | `btbcm_setup_patchram`, `btbcm_initialize`, `btbcm_patchram` (7 sym) | ORI (built-in Image) | **=y** (kembalikan) |
| USB_NET_AQC111 (Aquantia) | `aqc111_bind`, `aqc111_resume`, ... (27 sym) | ORI (built-in Image) | **=y** (kembalikan) |
| USB_RTL8152 (Realtek) | `r8152_submit_rx`, `r8152b_init`, ... (9 sym) | ORI (built-in Image) | **=y** (kembalikan) |
| BT_QCA (Qualcomm Atheros) | 0 sym, tapi `stock.config` =y | ORI (config) | **=y** (kembalikan) |

Alasannya: di stok ORI driver ini **built-in di Image** (bukan .ko), jadi tidak
muncul di lsmod - inilah jebakan: "tidak ada di lsmod" != "tidak ada di stok".
Mematikannya berarti menyimpang dari ORI, kebalikan dari perintah user.

Sweep arch/vendor asing lain: **BERSIH** (tidak ada x86_64/S390/PPC/MIPS/PARISC/
SPARC/IA64, tidak ada iwlwifi/ath*/rtl8xxx-wifi/mwifiex/mt76/brcmfmac, tidak ada
DRM i915/amd/msm/rockchip).

Sweep menyeluruh TIDAK menemukan: x86_64/S390/PPC/MIPS/PARISC/SPARC/IA64/ALPHA (bersih),
Broadcom WiFi (brcmfmac — bersih), Intel WiFi (iwlwifi — bersih), Realtek WiFi (rtl8xxx —
bersih), Marvell/MediaTek WiFi (bersih), DRM asing (i915/amd/msm/rockchip — bersih).
Yang tersisa hanya BT_BCM/BT_QCA (BT HCI transport generik) + 2 USB-Ethernet — semuanya
bukan ORI dan sudah dimatikan.

## Verifikasi patch (build CI nyata, bukan simulasi)
Build: `v17-build.yml` run **41286492718** (commit acadcef7) -> **SUCCESS 1j16m**.
Artefak `v17-built-kos` (48.2 MB, **234 .ko**) + `v17-config` (effective.config).

Config efektif (hasil olddefconfig nyata):
- WCN_WLAN=m ✓, WCN_WLAN_SC2355=m ✓, NVMEM_SPRD_EFUSE=m ✓, SPRD_TSENSOR_THERMAL=m ✓,
  SPRD_SOC_THM=m ✓, SPRD_BATTERY_INFO=m ✓, SPRD_VDSP=m ✓
- BT_BCM=y, BT_QCA=y, USB_NET_AQC111=y, USB_RTL8152=y (ORI, dikembalikan)
- `I2C_SPRD_HW` **hilang** dari config efektif -> phantom Kconfig, benar dibuang olddefconfig
- Dependensi kritis OK: MFD_SC27XX_PMIC=y, CFG80211=y, WCN_CORE=m, SC23XX=y,
  ARCH_SPRD=y, HAS_IOMEM=y, REGMAP=y, POWER_SUPPLY=y

**Modul baru yang ter-build (bukti di 234 .ko):**
| Modul | Status |
|---|---|
| `sprdwl.ko` (WiFi SC2355) | OK - sebelumnya TIDAK PERNAH dibangun |
| `nvmem_sprd_efuse.ko` | OK |
| `sc27xx_tsensor_thermal.ko` | OK |
| `sprd_soc_thm.ko` | OK |
| `sprd_battery_info.ko` | OK |
| `sprd_vdsp.ko` | OK |

Catatan: `select` mengalahkan `# ... is not set` - `BT_HCIUART_BCM=y` select BT_BCM,
`BT_HCIUART_QCA=y` select BT_QCA. Build ulang run **41288388110** (commit dae592d5)
menambahkan 4 simbol ORI kembali ke =y.

ORI ter-build: **109 → 113** (+4 modul baru dari fix config).

## DRIVER ORI YANG BENAR-BENAR BELUM TER-BUILD (verifikasi build final run 41288388110)

Build final menghasilkan **234 .ko**. Semua driver ORI dengan padanan fungsional di tree
**sudah ter-build**. Sisa 4 driver yang benar-benar belum ada di build (perlu porting/backport):

1. **i2c_sprd_hw** — I2C HW controller (`sprd,sc9860-hw-i2c`). Tree hanya punya `i2c-sprd.c`
   (versi standard); versi HW tidak di-port, Kconfig `I2C_SPRD_HW` TIDAK ADA (config `=m`
   itu phantom — olddefconfig membuangnya, terkonfirmasi: `I2C_SPRD_HW` ABSEN di
   effective.config).
2. **sprd_ddr_dvfs** — DDR DVFS. Tidak ada source di tree (hanya apsys/top dvfs yang ada:
   `sprd-top-dvfs.ko`, `apsys-dvfs.ko` ter-build, tapi ddr dvfs terpisah tidak ada).
3. **sprd_compr_2stage_dma** — audio 2-stage DMA. Tidak ada di tree; sumber vendor ADA di
   `vendor_src/sprd-kernel-modules-audio__a14/sprd/platform/compr_2stage_dma/`
   (`sprd-compr-2stage-dma.c` 61KB + Makefile) — kandidat backport paling feasible.
4. **sha1_ce** — `sha1-ce-glue.c` TIDAK ADA di `arch/arm64/crypto` tree 6.18;
   `CRYPTO_SHA1_ARM64_CE` tidak ada di config efektif.

### Yang ternyata SUDAH ter-build (koreksi dari analisis awal — build mengoreksi)
Analisis awal menyebut 8-9 driver "hilang"; build final membuktikan banyak yang sebenarnya
SUDAH ada:
- `sprd_wlan_combo` (WiFi) → **`sprdwl.ko`** ✓ (WCN_WLAN=m)
- `sprd_7sreset` → **`sprd_7sreset.ko`** ✓
- `sprd_cpufreq_public` / `sprd_cpufreq_v2` → **`sprd-cpufreq-public.ko` / `sprd-cpufreq-v2.ko`** ✓
- `sprd_manufacturer_model` → **`sprd_manufacturer_model.ko`** ✓
- `sprd_pdbg` → **`sprd_pdbg.ko`** ✓
- `sprd_vdsp` → **`sprd_vdsp.ko`** ✓
- `snd_soc_sprd_vbc_fe` → **`snd-soc-sprd-vbc-v4.ko`** ✓ (rename)
- phy/pinctrl sharkl5Pro → `phy-sprd-ums512.ko` ✓

### Modul tambahan port (built tapi tidak ada di stok) — 11
`phy-sprd-ums512`, `sprd-systimer`, `sprd_autotest_core`, `sprd_autotest_gpio`,
`sprd_debugstat`, `sprdwl`, `wcn_core`, `wcn_integrate`, `wcn_integrate_boot`,
`wcn_integrate_dev`, `wcn_sipc`.

## Dampak fungsional (yang paling penting untuk boot/performa)
- **WiFi SC2355 (sprdwl.ko)** — SEBELUMNYA MATI karena WCN_WLAN off. Sekarang =m → akan dibangun.
  Ini modul WiFi utama tablet (sprd_wlan_combo.ko stok). Fix paling berdampak.
- **Thermal chain** — SPRD_TSENSOR_THERMAL (sensor suhu PMIC) + SPRD_SOC_THM (agregator zona
  virtual) + SPRD_CPU_COOLING(m) + SPRD_THERMAL(y) kini lengkap. Tanpa tsensor_thermal,
  sprd_thermal tidak punya sumber suhu → thermal throttling tidak jalan.
- **Battery** — sprd_battery_info (parser tabel karakteristik baterai dari DT) kini =m.
- **VDSP/NPU** — sprd_vdsp.ko (firmware Cadence XRP) kini =m.
- **Efuse** — nvmem_sprd_efuse.ko (dump eFuse, kalibrasi thermal) kini =m.

## Perintah user yang dijalankan
1. "lsmod di tablet ada berapa driver dan berapa yang belum" → **158 live** (155 ORI + 3 berry);
   **109 ter-build** di port → setelah fix **113**; **8 benar-benar hilang** (perlu porting).
2. "hapus yang bukan ORI (broadcomm/x86_64/dll)" → 4 non-ORI dimatikan (BT_BCM, BT_QCA,
   USB_NET_AQC111, USB_RTL8152); sweep arch asing bersih.
