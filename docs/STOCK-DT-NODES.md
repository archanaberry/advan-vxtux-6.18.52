# Node DT stok untuk blob yang belum ter-port

Sumber: `donors/stock_initramfs/stock.dtb`. Dibuat oleh
`donors/dt_nodes_for_blobs.py`. Node di bawah **ada di stok** tetapi
tidak ada di DTS vxtux (lihat `docs/RE-PORT-DOSSIER.md`), jadi harus
ikut di-port bersama kodenya.

## `sprd,cpufreq-v2` (untuk `sprd-cpufreq-v2.ko`)

path: `/sys/firmware/devicetree/base//soc/aon/apcpu-dvfs`

```dts
apcpu-dvfs {
	compatible = "sprd,cpufreq-v2";
	phandle = <0x133>;
};
```

## `sprd,voltage-margin` (untuk `sprd-cpufreq-v2.ko`)

Tidak ditemukan di DTB stok.

## `sprd,debugstat` (untuk `sprd_power_stat.ko`)

path: `/sys/firmware/devicetree/base//debugstat`

```dts
debugstat {
	compatible = "sprd,debugstat";
	soc_slp_cnt_reg = <0x2 0x784 0xff>;
	phandle = <0x168>;
};
```

## `sprd,debuglog` (untuk `sprd_pdbg.ko`)

path: `/sys/firmware/devicetree/base//debuglog`

```dts
debuglog {
	compatible = "sprd,debuglog";
	sprd,pdbg-irq-domain-gic = "GICv3";
	sprd,pdbg-irq-domain-gpio = "sprd-gpio";
	sprd,pdbg-irq-domain-ana = "spi4.0";
	sprd,pdbg-irq-domain-ana-eic = "32100000.spi:pmic@0:gpio@280";
	sprd,pdbg-irq-domain-ap-eic-sync = "320200c0.gpio";
	phandle = <0x167>;
};
```

## `sprd,pdbg-irq-domain-gpio` (untuk `sprd_pdbg.ko`)

Tidak ditemukan di DTB stok.

## `sprd,pdbg-irq-domain-gic` (untuk `sprd_pdbg.ko`)

Tidak ditemukan di DTB stok.

## `sprd,pdbg-irq-domain-ana` (untuk `sprd_pdbg.ko`)

Tidak ditemukan di DTB stok.

## `sprd,sc2730-7sreset` (untuk `sprd_7sreset.ko`)

path: `/sys/firmware/devicetree/base//soc/aon/spi@32100000/pmic@0/hw-reset@1bc0`

```dts
hw-reset@1bc0 {
	compatible = <0x73707264 0x2c736332 0x3778782d 0x37737265 0x73657400 0x73707264 0x2c736332 0x3733302d 0x37737265 0x73657400>;
	reg = <0x1bf8 0x1bc0>;
	phandle = <0x130>;
};
```

## `unisoc,sc2730-audio-codec` (untuk `snd-soc-sprd-codec-sc2730-power.ko`)

path: `/sys/firmware/devicetree/base//soc/aon/spi@32100000/pmic@0/audio-codec@1000`

```dts
audio-codec@1000 {
	compatible = <0x756e6973 0x6f632c73 0x63323733 0x302d6175 0x64696f2d 0x636f6465 0x6300756e 0x69736f63 0x2c736332 0x3733312d 0x61756469 0x6f2d636f 0x64656300>;
	#sound-dai-cells = <0x1>;
	reg = <0x1000>;
	sprd,syscon-pmu-apb = <0x2>;
	nvmem-cells = <0x3a 0x3b 0x34 0x3c>;
	nvmem-cell-names = 6175645f70616273745f7663616c5f6566757365006e65675f63705f6566757365006667755f3470325f656675736500636f6465635f70726f647563745f696e666f00;
	status = "okay";
	hp-use-inter-pa = <0x1>;
	fixed-sample-rate = <0xbb80 0xbb80 0xbb80>;
	digital-codec = <0x3d>;
	phandle = <0xb4>;
};
```

## `sprd,iq` (untuk `sprd_iq.ko`)

path: `/sys/firmware/devicetree/base//reserved-memory/iq-mem@90000000`

```dts
iq-mem@90000000 {
	compatible = "sprd,iq-mem";
	reg = <0x0 0x90000000 0x0 0x4000000>;
	phandle = <0xba>;
};
```

## `sprd,iq` (untuk `sprd_iq.ko`)

path: `/sys/firmware/devicetree/base//sprd-iq`

```dts
sprd-iq {
	compatible = "sprd,iq";
	sprd,region = <0xba>;
	sprd,mapping-offs = <0x0>;
};
```

## `sprd,sc27xx-syscon` (untuk `bq2560x-charger.ko`)

path: `/sys/firmware/devicetree/base//soc/aon/spi@32100000/pmic@0/syscon@1800`

```dts
syscon@1800 {
	compatible = 737072642c7363323733312d737973636f6e00737072642c7363323778782d737973636f6e00;
	reg = <0x1800 0x1ba0 0x1b98>;
	phandle = <0x10b>;
};
```

