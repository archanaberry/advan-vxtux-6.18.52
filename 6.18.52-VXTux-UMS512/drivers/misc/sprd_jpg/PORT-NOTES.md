# sprd_jpg — 5.4 → 6.18.52 port notes

Donor: `work/donors_20260930/iscle_ums512/drivers/misc/sprd_jpg/` (5.4,
same UMS512 SoC). The blob-ledger row `jpg NOT-PORTED — "no JPEG codec
source anywhere under drivers/media/"` was **wrong**: the source exists, under
`drivers/misc/`, in that donor only. `drivers/media/` was the wrong place to
look.

Files: `sprd_jpg.c` (646 lines), `sprd_jpg_common.c` (686),
`sprd_jpg_common.h` (141), plus `drivers/misc/Makefile`'s
`obj-$(CONFIG_SPRD_JPG) += sprd_jpg/`. The uapi header
`include/uapi/video/sprd_jpg.h` was already recovered into the tree by an
earlier tick.

## 5.4 → 6.18 deltas applied

| Donor | 6.18 | Why |
|---|---|---|
| `static int jpg_remove(struct platform_device *)` | `static void jpg_remove(...)` | `.remove` is void in 6.18 |
| `free_irq(jpg_hw_dev.irq, ...)` in remove | **removed** | the IRQ came from `devm_request_irq()`; devres already frees it on unbind, so the explicit `free_irq()` was an unbalanced teardown |
| `syscon_regmap_lookup_by_name(np, "aon_apb_eb")` / `syscon_get_args_by_name(np, "reset", 2, args)` | `of_parse_phandle_with_args(np, prop, NULL, 0, &a)` on `aon-apb-eb-syscon` / `reset-syscon` | both helpers are gone upstream, **and** their 5.4 vendor semantics were syscon-names identifiers, not property names — see below |
| `#if IS_ENABLED(CONFIG_SPRD_JPG_CALL_VSP_PW_DOMAIN)` … 4 more | single explicit no-op body | none of those five symbols exists in this tree, so all five branches folded to 0 and the donor's stub branch was what compiled anyway |
| `#ifdef CONFIG_COMPAT` compat ioctl block (73 lines) | deleted | `CONFIG_COMPAT is not set` in `.config`; arm64-only board |
| `clk_prepare_enable(jpg_hw_dev->jpg_domain_eb)` etc, unguarded | `IS_ERR_OR_NULL()` gate before every enable/disable/`clk_set_parent` | `jpg_get_mm_clk()` tolerates *every* `devm_clk_get()` failure and stores NULL, so the donor's unconditional `clk_prepare_enable(NULL)` was a live NULL dereference on any DT missing a clock |
| `for (i = 0; i < ARRAY_SIZE(jpg_clk_src); i++) … clock_name_map[j++]` | bounds-check `j`, and record `jpg_hw_dev.clk_num` | the 8-entry `jpg_clk_src[]` is filtered by what the DT actually has; the stock DT has 4, so entries 4..7 stay zeroed and `find_jpg_freq_level()`/`jpg_get_clk_src_name()` indexed them by `max_freq_level` |
| `of_parse_phandle(np, "jpg_qos", 0)` | same, on `sprd,qos` | that is the property name the donor DTS (`roc1.dtsi`, `sharkl5Pro.dtsi`) uses. **Not present in the stock UMS512 node**, so the QoS path stays off — reported, not invented |
| `sprd_jpg_pw_on()` result discarded, `ret = 0` forced after | return value logged, not forced | cosmetic |
| `compat_ptr` / `compat_alloc_user_space` | gone with the compat block | — |

`sprd_iommu_attach_device()` was declared in `include/linux/sprd_iommu.h`
with **no definition anywhere in the tree** — calling it was a guaranteed
undefined symbol at link. Implemented in `drivers/iommu/sprd-iommu.c` as
"does this device have an iommu domain", which is the question every caller
is actually asking, and the answer the clients' physical-address fallback
depends on.

## The DT-layout trap (this is the one that would have compiled clean and done nothing)

The 5.4 driver reached its two registers through the syscon-namespace
identifiers `"aon_apb_eb"` and `"reset"`. The stock UMS512 DTB
(`/data/local/tmp/dtsx/fdt.dts`, node `jpg-codec@62700000`) has **no**
`syscons`/`syscon-names` pair at all — it has two independent phandle
properties:

```
reset-syscon      = <0x13 0x04 0x02>
aon-apb-eb-syscon = <0x0c 0x00 0x200>
```

Reusing the `syscons`/`syscon-names` list shim that
`drivers/sound/soc/sprd/agdsp_access/vxtux-syscon-shim.h` provides — which is
exactly what `sprd_camsys_pw_domain` wrongly did — makes all three lookups
return `-ENODEV`, leaves `regs[]` zeroed, and the driver still probes
"successfully". So the port does `of_parse_phandle_with_args()` on the two
property names directly, and `jpg_probe()` now returns `-ENODEV` from
`JPG_RESET` rather than calling `regmap_update_bits(NULL, …)`.

Same class of defect as the `sprd_camsys_pw_domain` invented-compatible bug:
a driver can be fully built, linked and present in `System.map` and still be
structurally unable to reach its hardware.

## Device-tree node

Added to `arch/arm64/boot/dts/sprd/ums512-1h10-vxtux.dts` under `&soc`,
reconstructed from the stock node. `compatible = "sprd,sharkl5pro-jpg"` **is**
in the driver's `of_match_table`, so this one can actually bind.

The IOMMU node is `compatible = "sprd,iommu-v1"` — the only string in
`drivers/iommu/sprd-iommu.c`'s match table. Note the pre-existing
`iommu_isp`/`iommu_dcam` nodes in the same file use `sprd,iommuvau-*`, which
**never matches** that driver; that is a separate open defect, not touched
here.

## Status

Compile + link + symbol presence only. No probe: `-M virt` cannot present
UMS512 silicon. The power-domain hooks (`sprd_jpg_pw_on/off`) are no-ops —
the VSP domain the donor would have used is still NOT-PORTED, and the camsys
domain is a different node with its own driver.