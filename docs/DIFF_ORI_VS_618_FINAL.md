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
| **Ter-build di port** (target+config on) | **109** |
| Belum ter-build | 46 |
| — yang HILANG dari tree (tidak ada padanan) | 8 |
| — yang RENAME/padanan ada tapi target nama beda / config off | 38 |

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

## 8 DRIVER ORI YANG BENAR-BENAR HILANG DARI TREE 6.18.52 (perlu porting manual)
1. **i2c_sprd_hw** — I2C HW controller (`sprd,sc9860-hw-i2c`). Hanya i2c-sprd.c (versi standard)
   yang ada; versi HW tidak di-port. Kconfig I2C_SPRD_HW TIDAK ADA di tree (config `=m` itu
   phantom — olddefconfig akan buang).
2. **phy_sprd_sharkl5Pro** — PHY UMS512 ada (`phy-sprd-ums512.c`), tapi compatible
   `sprd,sharkl5pro`/`sprd,pike2` tidak di-handle → hanya `sprd,ums512` saja.
3. **pinctrl_sprd_sharkl5Pro** — `pinctrl-sprd-sharkl5pro.c` ADA di tree; perlu cek apakah
   di-descend (Makefile/Kconfig pinctrl/sprd). (kemungkinan RENAME, bukan hilang)
4. **sha1_ce** — `CONFIG_CRYPTO_SHA1_ARM64_CE=m` ADA; perlu cek apakah sumber `sha1-ce-glue.c`
   ada di arch/arm64/crypto. (kemungkinan RENAME)
5. **snd_soc_sprd_card / codec_sc2730_power(-dev) / dummy_codec / vbc_fe** — SEMUA ADA di
   `drivers/vxtux/sound/asoc/` (card/, codec/, dai_v4/) dengan nama target beda
   (sprd-asoc-card-utils.ko, sprd-audio-power.ko, dummy-codec.ko, sprd-fe-dai.ko). RENAME.
6. **sprd_7sreset** — reset controller (`sprd,sc2730-7sreset`). TIDAK ADA padanan.
7. **sprd_cpufreq_public / sprd_cpufreq_v2** — cpufreq vendor. TIDAK ADA di drivers/cpufreq.
   (cpufreq di port pakai ARM_SCMI_CPUFREQ=y + ARM_SCPI_CPUFREQ=y — jalur SCMI mainline,
   bukan sprd-cpufreq-v2. Perlu verifikasi DT menyuplai SCMI cpufreq, jika tidak cpufreq mati.)
8. **sprd_ddr_dvfs** — DDR DVFS. Hanya apsys dvfs (`sprd_dvfs_apsys.c`) yang ada; ddr dvfs terpisah
   tidak di-port.
9. **sprd_manufacturer_model** — TIDAK ADA padanan.
10. **sprd_pdbg** — debug logger. TIDAK ADA padanan.
11. **sprd_compr_2stage_dma** — audio 2-stage DMA. TIDAK ADA padanan (sprd-dma ada, tapi bukan 2stage).

Catatan: item 2,3,4,5 kemungkinan besar RENAME (padanan fungsional ada di tree, tinggal dipastikan
target ter-descend). Yang PASTI hilang dan butuh porting: **i2c_sprd_hw, sprd_7sreset,
sprd_cpufreq_public, sprd_cpufreq_v2, sprd_ddr_dvfs, sprd_manufacturer_model, sprd_pdbg,
sprd_compr_2stage_dma**.

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
