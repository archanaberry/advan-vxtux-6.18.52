// SPDX-License-Identifier: GPL-2.0
/*
 * drivers/vxtux/vendor_abi/vendor_abi_arm64.c
 *
 * Penyedia ABI global arm64 5.4 untuk modul blob vendor UMS512.
 *
 * MASALAH (gerbang ronde-11, kelas ARM64-GLOBAL-GONE)
 *   42 modul blob meng-UND tiga simbol kapabilitas arm64 5.4
 *   (cpu_hwcaps x30, cpu_hwcap_keys x42, arm64_const_caps_ready x42),
 *   10 modul meng-UND kimage_vaddr, 5 modul meng-UND cpu_number.
 *   Di 6.18 keempatnya HILANG, bukan sekadar berganti nama:
 *     - capabilitas pindah ke bitmap internal `system_cpucaps` dengan
 *       penomoran BARU (arch/arm64/tools/cpucaps menomori alfabetis,
 *       dimulai dari ALWAYS_BOOT=0; 5.4 menomori per enum fitur);
 *     - kimage_vaddr tidak ada lagi (grep 0x di upstream 6.18);
 *     - cpu_number hanya ada di x86 (arch/x86/kernel/setup_percpu.c).
 *
 * BUKTI 5.4 (oracle: gpl-source/fork_t618/... — pohon kernel 5.4 utuh)
 *   [A1] arch/arm64/include/asm/cpufeature.h:338-340
 *	  extern DECLARE_BITMAP(cpu_hwcaps, ARM64_NCAPS);
 *	  extern struct static_key_false cpu_hwcap_keys[ARM64_NCAPS];
 *	  extern struct static_key_false arm64_const_caps_ready;
 *	  ARM64_NCAPS = 28  (arch/arm64/include/asm/cpucaps.h:49)
 *   [A2] idem :351-371 — cpus_have_const_cap():
 *	  if (static_branch_likely(&arm64_const_caps_ready))
 *		  return static_branch_unlikely(&cpu_hwcap_keys[num]);
 *	  else
 *		  return test_bit(num, cpu_hwcaps);
 *   [A3] arch/arm64/kernel/cpufeature.c:51,73,1544 — ketiganya EXPORT_SYMBOL
 *	  (tanpa _GPL: modul vendor proprietary memang mengimpornya).
 *   [A4] arch/arm64/kernel/head.S:387 — ENTRY(kimage_vaddr)
 *					     .quad _text - TEXT_OFFSET
 *	  memory.h:186 "the virtual base of the kernel image (minus TEXT_OFFSET)";
 *	  TEXT_OFFSET 5.4 = 0x80000 (arch/arm64/Makefile:128).
 *	  Di 6.18 tidak ada TEXT_OFFSET dan vmlinux.lds.S:394 meng-ASSERT
 *	  (_text == KIMAGE_VADDR) → nilai ekuivalen = VA runtime &_text.
 *   [A5] arch/arm64/kernel/smp.c:71-72,716-717
 *	  DEFINE_PER_CPU_READ_MOSTLY(int, cpu_number);
 *	  EXPORT_PER_CPU_SYMBOL(cpu_number);
 *	  ... per_cpu(cpu_number, cpu) = cpu;
 *
 * RE PEMAKAI DI BLOB (bukan asumsi; alat tools/re/re_cpucaps_bits.py)
 *   Dari 30 modul yang merujuk cpu_hwcaps, HANYA dua bit yang benar-benar
 *   dibaca (relokasi → ldr → tbz). Keluaran alat tersimpan di
 *   device_artifacts/vendor-abi/cpucaps_bits.txt:
 *     bit  4 (ARM64_HAS_PAN)                x27 modul
 *     bit 23 (ARM64_UNMAP_KERNEL_AT_EL0)    x10 modul
 *   Sisa 26 baris tetap diisi (bukan nol buta) dari padanan 6.18 bila ada,
 *   supaya bitmap tidak berbohong bila kelak ada pemakai lain.
 *
 * KEPUTUSAN BENTUK
 *   1. `arm64_const_caps_ready` sengaja DIBIARKAN MATI (default static key
 *      false). Akibatnya, sesuai [A2], setiap cpus_have_const_cap() di blob
 *      mengambil jalur `test_bit(num, cpu_hwcaps)` — satu sumber kebenaran
 *      (bitmap kita), tanpa bergantung pada stride/isi struct static_key
 *      versi 6.18 maupun urutan patching jump-label modul. `cpu_hwcap_keys`
 *      tetap disediakan agar relokasi jump-table blob resolve saat load.
 *   2. Bitmap = 1 word (ARM64_NCAPS 28 <= 64) — persis ukuran DECLARE_BITMAP
 *      hasil BITS_TO_LONGS(28) di 5.4, jadi `ldr x, [cpu_hwcaps]` blob sah.
 *   3. Dibangun BUILT-IN: kimage_vaddr butuh &_text (simbol image) dan
 *      cpu_number butuh area per-CPU kernel (lihat Kconfig).
 *
 * BATAS JUJUR
 *   - Nilai 27 bit non-pemakai berasal dari padanan 6.18; bila 6.18 sudah
 *     tidak punya padanannya, nilainya diambil dari kriteria 5.4 yang
 *     bersangkutan dan ditulis apa adanya di tabel [T] (0 + alasan).
 *   - Tidak ada blob yang dijalankan di sini: ini penyedia simbol statis.
 */

#define pr_fmt(fmt) "vendor_abi: " fmt

#include <linux/bitmap.h>
#include <linux/bitops.h>
#include <linux/cpumask.h>
#include <linux/export.h>
#include <linux/init.h>
#include <linux/jump_label.h>
#include <linux/kernel.h>
#include <linux/percpu.h>
#include <asm-generic/sections.h>
#include <asm/cpufeature.h>

#include "vendor_abi.h"

/* [A1] ARM64_NCAPS 5.4. */
#define ARM64_NCAPS_54	28

/* ---- tiga global kapabilitas 5.4 (nama & tipe persis seperti 5.4) [A1] --- */
DECLARE_BITMAP(cpu_hwcaps, ARM64_NCAPS_54);
EXPORT_SYMBOL(cpu_hwcaps);

struct static_key_false cpu_hwcap_keys[ARM64_NCAPS_54];
EXPORT_SYMBOL(cpu_hwcap_keys);

struct static_key_false arm64_const_caps_ready;
EXPORT_SYMBOL(arm64_const_caps_ready);

/* ---- kimage_vaddr 5.4 [A4] ---------------------------------------------- */
u64 kimage_vaddr;
EXPORT_SYMBOL(kimage_vaddr);

/* ---- cpu_number per-CPU 5.4 [A5] --------------------------------------- */
DEFINE_PER_CPU_READ_MOSTLY(int, cpu_number);
EXPORT_PER_CPU_SYMBOL(cpu_number);

/*
 * [T] Tabel 28 baris kapabilitas 5.4 → sumber nilai di 6.18.
 *     Dipakai dua hal: (1) dokumentasi yang bisa diperiksa, (2) diparsing
 *     gerbang selftest (harus 28 baris, bit 0..27 tepat sekali).
 *
 *     bit  nama 5.4                                sumber nilai
 */
struct abi_cap54_row {
	u8		bit;
	const char	*name;
	const char	*source;
};

static const struct abi_cap54_row abi_caps54[] __initconst = {
	{  0, "ARM64_WORKAROUND_CLEAN_CACHE",
	  "6.18 nama sama: cpus_have_cap(ARM64_WORKAROUND_CLEAN_CACHE)" },
	{  1, "ARM64_WORKAROUND_DEVICE_LOAD_ACQUIRE",
	  "6.18 nama sama: cpus_have_cap(ARM64_WORKAROUND_DEVICE_LOAD_ACQUIRE)" },
	{  2, "ARM64_WORKAROUND_845719",
	  "6.18 nama sama: cpus_have_cap(ARM64_WORKAROUND_845719)" },
	{  3, "ARM64_HAS_SYSREG_GIC_CPUIF",
	  "6.18 berganti nama: cpus_have_cap(ARM64_HAS_GICV3_CPUIF)" },
	{  4, "ARM64_HAS_PAN (DIBACA BLOB)",
	  "6.18: system_uses_hw_pan()" },
	{  5, "ARM64_HAS_LSE_ATOMICS",
	  "6.18 nama sama: cpus_have_cap(ARM64_HAS_LSE_ATOMICS)" },
	{  6, "ARM64_WORKAROUND_CAVIUM_23154",
	  "6.18 nama sama: cpus_have_cap(ARM64_WORKAROUND_CAVIUM_23154)" },
	{  7, "ARM64_WORKAROUND_834220",
	  "6.18 nama sama: cpus_have_cap(ARM64_WORKAROUND_834220)" },
	{  8, "ARM64_HAS_NO_HW_PREFETCH",
	  "6.18 tidak punya; 5.4 = MIDR ThunderX pass 1.x/2.x (cpufeature.c:795)"
	  " → T618 bukan ThunderX: 0" },
	{  9, "ARM64_HAS_UAO",
	  "6.18 menghapus kapabilitas UAO (ID_AA64MMFR2_EL1.UAO tidak lagi"
	  " diekspos) → 0 konservatif: hanya mematikan jalur optimasi UAO" },
	{ 10, "ARM64_ALT_PAN_NOT_UAO",
	  "turunan [A2/cpufeature.c:1063]: bit4 && !bit9" },
	{ 11, "ARM64_HAS_VIRT_HOST_EXTN",
	  "6.18 nama sama: cpus_have_cap(ARM64_HAS_VIRT_HOST_EXTN)" },
	{ 12, "ARM64_WORKAROUND_CAVIUM_27456",
	  "6.18 nama sama: cpus_have_cap(ARM64_WORKAROUND_CAVIUM_27456)" },
	{ 13, "ARM64_HAS_32BIT_EL0",
	  "6.18 helper: system_supports_32bit_el0()" },
	{ 14, "ARM64_HYP_OFFSET_LOW",
	  "6.18 tidak punya (5.4 = idmap HYP tak bentrok && !EL2,"
	  " cpufeature.c:805); bit tidak dipakai blob → 0" },
	{ 15, "ARM64_MISMATCHED_CACHE_LINE_SIZE",
	  "6.18 tidak punya (cap dihapus); A75/A55 CTR_EL0 seragam → 0" },
	{ 16, "ARM64_HAS_NO_FPSIMD",
	  "6.18 cap terbalik: !cpus_have_cap(ARM64_HAS_FPSIMD)"
	  " (setara cpufeature.c:741 system_supports_fpsimd())" },
	{ 17, "ARM64_WORKAROUND_REPEAT_TLBI",
	  "6.18 nama sama: cpus_have_cap(ARM64_WORKAROUND_REPEAT_TLBI)" },
	{ 18, "ARM64_WORKAROUND_QCOM_FALKOR_E1003",
	  "6.18 nama sama: cpus_have_cap(ARM64_WORKAROUND_QCOM_FALKOR_E1003)" },
	{ 19, "ARM64_WORKAROUND_858921",
	  "6.18 nama sama: cpus_have_cap(ARM64_WORKAROUND_858921)" },
	{ 20, "ARM64_WORKAROUND_CAVIUM_30115",
	  "6.18 nama sama: cpus_have_cap(ARM64_WORKAROUND_CAVIUM_30115)" },
	{ 21, "ARM64_HAS_DCPOP",
	  "6.18 nama sama: cpus_have_cap(ARM64_HAS_DCPOP)" },
	{ 22, "(5.4 tidak memakai nomor ini — lubang di cpucaps.h 5.4)",
	  "selalu 0" },
	{ 23, "ARM64_UNMAP_KERNEL_AT_EL0 (DIBACA BLOB)",
	  "6.18 nama sama: cpus_have_cap(ARM64_UNMAP_KERNEL_AT_EL0)"
	  " (= arm64_kernel_unmapped_at_el0())" },
	{ 24, "ARM64_HARDEN_BRANCH_PREDICTOR",
	  "6.18 berganti nama: cpus_have_cap(ARM64_SPECTRE_V2)" },
	{ 25, "ARM64_SSBD",
	  "5.4 sendiri tidak pernah menyalakannya (grep: 0 entri"
	  " `.capability = ARM64_SSBD` di cpufeature.c/cpu_errata.c) → 0" },
	{ 26, "ARM64_MISMATCHED_CACHE_TYPE",
	  "6.18 nama sama: cpus_have_cap(ARM64_MISMATCHED_CACHE_TYPE)" },
	{ 27, "ARM64_SSBS",
	  "6.18 nama sama: cpus_have_cap(ARM64_SSBS)" },
};

#define ABI_CAP54_MAX_BIT	(ARM64_NCAPS_54 - 1)
#define ABI_CAP54_EXPECT_ROWS	ARM64_NCAPS_54

/* Cek bentuk tabel saat kompilasi: 28 baris (isi/bit dicek gerbang selftest). */
static_assert(ARRAY_SIZE(abi_caps54) == ABI_CAP54_EXPECT_ROWS,
	      "tabel kapabilitas 5.4 harus 28 baris");

static void __init vendor_abi_fill_hwcaps(void)
{
	unsigned long bits = 0;
	int cpu;

	/* Padanan langsung 6.18 (nama sama). */
	if (cpus_have_cap(ARM64_WORKAROUND_CLEAN_CACHE))
		__set_bit(0, &bits);
	if (cpus_have_cap(ARM64_WORKAROUND_DEVICE_LOAD_ACQUIRE))
		__set_bit(1, &bits);
	if (cpus_have_cap(ARM64_WORKAROUND_845719))
		__set_bit(2, &bits);
	/* Berganti nama di 6.18. */
	if (cpus_have_cap(ARM64_HAS_GICV3_CPUIF))
		__set_bit(3, &bits);
	if (system_uses_hw_pan())
		__set_bit(4, &bits);
	if (cpus_have_cap(ARM64_HAS_LSE_ATOMICS))
		__set_bit(5, &bits);
	if (cpus_have_cap(ARM64_WORKAROUND_CAVIUM_23154))
		__set_bit(6, &bits);
	if (cpus_have_cap(ARM64_WORKAROUND_834220))
		__set_bit(7, &bits);
	/* bit 8 (NO_HW_PREFETCH, ThunderX) dan bit 9 (UAO): lihat tabel. */
	if (test_bit(4, &bits) && !test_bit(9, &bits))
		__set_bit(10, &bits);	/* ALT_PAN_NOT_UAO = bit4 && !bit9 */
	if (cpus_have_cap(ARM64_HAS_VIRT_HOST_EXTN))
		__set_bit(11, &bits);
	if (cpus_have_cap(ARM64_WORKAROUND_CAVIUM_27456))
		__set_bit(12, &bits);
	if (system_supports_32bit_el0())
		__set_bit(13, &bits);
	/* bit 14 (HYP_OFFSET_LOW) dan bit 15 (MISMATCHED_CACHE_LINE_SIZE). */
	if (!cpus_have_cap(ARM64_HAS_FPSIMD))
		__set_bit(16, &bits);
	if (cpus_have_cap(ARM64_WORKAROUND_REPEAT_TLBI))
		__set_bit(17, &bits);
	if (cpus_have_cap(ARM64_WORKAROUND_QCOM_FALKOR_E1003))
		__set_bit(18, &bits);
	if (cpus_have_cap(ARM64_WORKAROUND_858921))
		__set_bit(19, &bits);
	if (cpus_have_cap(ARM64_WORKAROUND_CAVIUM_30115))
		__set_bit(20, &bits);
	if (cpus_have_cap(ARM64_HAS_DCPOP))
		__set_bit(21, &bits);
	/* bit 22 tidak dipakai 5.4. */
	if (cpus_have_cap(ARM64_UNMAP_KERNEL_AT_EL0))
		__set_bit(23, &bits);
	if (cpus_have_cap(ARM64_SPECTRE_V2))
		__set_bit(24, &bits);
	/* bit 25 (SSBD) tidak pernah diset 5.4. */
	if (cpus_have_cap(ARM64_MISMATCHED_CACHE_TYPE))
		__set_bit(26, &bits);
	if (cpus_have_cap(ARM64_SSBS))
		__set_bit(27, &bits);

	WRITE_ONCE(cpu_hwcaps[0], bits);

	pr_info("cpu_hwcaps 5.4 = 0x%016lx (bit dibaca blob: 4=%d 23=%d)\n",
		bits, test_bit(4, &bits) ? 1 : 0, test_bit(23, &bits) ? 1 : 0);

	/*
	 * cpu_number = nomor CPU pada salinannya masing-masing [A5]. Di 5.4
	 * pengisian terjadi saat CPU kedua dan seterusnya naik (smp.c:716);
	 * di sini area per-CPU sudah tersedia sejak setup_per_cpu_areas(),
	 * jadi seluruh CPU possible diisi sekali di initcall.
	 */
	for_each_possible_cpu(cpu)
		per_cpu(cpu_number, cpu) = cpu;
}

static int __init vendor_abi_init(void)
{
	/*
	 * kimage_vaddr: ekuivalen [A4] — VA runtime basis image kernel.
	 * 5.4: _text - TEXT_OFFSET (0x80000); 6.18: _text == KIMAGE_VADDR.
	 */
	WRITE_ONCE(kimage_vaddr, (u64)(unsigned long)_text);

	/* Tabel [T] harus menutup 0..27 tanpa lubang kecuali bit 22. */
	{
		unsigned long seen = 0;
		unsigned int i, rows = ARRAY_SIZE(abi_caps54);

		for (i = 0; i < rows; i++)
			__set_bit(abi_caps54[i].bit, &seen);
		if (!test_bit(22, &seen) || bitmap_weight(&seen, ARM64_NCAPS_54) !=
		    ARM64_NCAPS_54 - 1)
			pr_warn("tabel [T] tidak konsisten (bit tertutup=%u)\n",
				bitmap_weight(&seen, ARM64_NCAPS_54));
	}

	vendor_abi_fill_hwcaps();

	pr_info("penyedia ABI arm64 5.4 aktif: kimage_vaddr=%#llx, "\
		"const_caps_ready=mati (blob memakai jalur bitmap)\n",
		(unsigned long long)kimage_vaddr);
	return 0;
}
/*
 * postcore_initcall: system_capabilities 6.18 sudah difinalkan jauh sebelum
 * initcall pertama (cpufeature.c:3843 setup_system_capabilities dipanggil dari
 * rangkaian setup_arch), sehingga cpus_have_cap()/system_supports_*() valid.
 */
postcore_initcall(vendor_abi_init);
