#!/usr/bin/env python3
"""Pastikan setiap regulator_desc SC2730 punya child DT yang akan dicocokkan.

regulator_of_get_init_node(dev, desc) mencari child container dengan
nama == desc->of_match, atau properti `regulator-compatible` == desc->of_match
(lihat drivers/regulator/of_regulator.c: regulator_of_get_init_node).
devm_regulator_register() mengembalikan -ENODEV bila tidak ketemu, dan
sc2730_regulator_probe() langsung berhenti -> SELURUH provider hilang, semua
consumer (vmmc eMMC, sdio, usb33, wifipa, codec) defer selamanya.
"""
import re
import sys

DRV = '6.18.52-VXTux-UMS512/drivers/regulator/sc2730-regulator.c'
DTS = '6.18.52-VXTux-UMS512/arch/arm64/boot/dts/sprd/sc2730.dtsi'

drv = open(DRV, encoding='utf-8').read()
dts = open(DTS, encoding='utf-8').read()

# SC2730_REGU_LINEAR(<id>, "<of_match>", ...
desc = re.findall(r'SC2730_REGU_LINEAR\(\s*([A-Z0-9_]+),\s*"([a-z0-9-]+)"', drv)
if not desc:
    sys.exit('FAILED: tidak ada deskriptor SC2730_REGU_LINEAR di %s' % DRV)

blk = dts[dts.index('regulators {'):]
blk = blk[:blk.index('\n\t\t};')]

nodes = {}
for m in re.finditer(r'\n\t\t\t(\w+): ([A-Za-z0-9_]+) \{\n(.*?)\n\t\t\t\};',
                     blk, re.S):
    label, name, body = m.groups()
    rc = re.search(r'regulator-compatible = "([^"]+)"', body)
    nodes[name] = (label, rc.group(1) if rc else None)

by_compat = {}
for name, (label, rc) in nodes.items():
    if rc:
        by_compat[rc] = name

print('deskriptor driver : %d' % len(desc))
print('child DT          : %d' % len(nodes))

missing = []
for _id, of_match in desc:
    if of_match in by_compat or of_match in nodes:
        continue
    missing.append((_id, of_match))

if missing:
    for _id, of_match in missing:
        print('MISSING  %-18s of_match=%-18s' % (_id, of_match))
    sys.exit('FAILED: %d rail tidak akan ter-match (probe -ENODEV)' % len(missing))

first_id, first_of = desc[0]
if first_of not in by_compat and first_of not in nodes:
    sys.exit('FAILED: rail pertama %s tidak match -> probe berhenti di situ'
             % first_of)

extra = sorted(set(nodes) - {n for _i, n in
                             [(i, o) for i, o in desc]} - set(by_compat.values()))
if extra:
    print('catatan: child tanpa deskriptor: %s' % ', '.join(extra))

print('OK: %d/%d of_match punya child DT (rail pertama: %s -> %s)'
      % (len(desc), len(desc), first_id, first_of))
for want in ('ldo-vddemmccore', 'ldo-vddsdcore', 'ldo-vddsdio', 'ldo-vddusb33'):
    print('   %-18s -> %s' % (want, by_compat.get(want, 'TIDAK ADA')))
