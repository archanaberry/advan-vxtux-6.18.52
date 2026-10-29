import subprocess
import sys

F = '6.18.52--VXTux-UMS512'
F = '6.18.52-VXTux-UMS512/drivers/regulator/of_regulator.c'

# kembalikan dulu ke versi HEAD (patch sebelumnya mengubah line ending)
subprocess.run(['git', 'checkout', '--', F], check=True)

data = open(F, 'rb').read()
if b'\r\n' in data:
    sys.exit('FAILED: berkas HEAD ternyata CRLF - hentikan')

HELPER = b"""/*
 * DT rail naming tolerance.
 *
 * The DT that this board really boots with lives in vendor_boot_a and was
 * written for the SPRD vendor tree: its rail nodes are named DCDC_CPU,
 * LDO_VDDEMMCCORE, ... (upper case, underscores), while the descriptors in
 * this kernel use the canonical lower-case dash form (dcdc-cpu).  A plain
 * strcmp therefore never matches, devm_regulator_register() fails with
 * -ENODEV on the FIRST descriptor, sc2730_regulator_probe() aborts, and every
 * consumer (eMMC vmmc, sdio, usb33, wifipa, codec) stays deferred forever.
 * Comparing case-insensitively with '_' == '-' makes the same descriptor bind
 * to either spelling, so the driver works with the stock DT and with ours.
 */
static bool regu_name_eq(const char *a, const char *b)
{
\tif (!a || !b)
\t\treturn false;

\tfor (;; a++, b++) {
\t\tchar ca = *a, cb = *b;

\t\tif (ca >= 'A' && ca <= 'Z')
\t\t\tca = ca - 'A' + 'a';
\t\tif (cb >= 'A' && cb <= 'Z')
\t\t\tcb = cb - 'A' + 'a';
\t\tif (ca == '_')
\t\t\tca = '-';
\t\tif (cb == '_')
\t\t\tcb = '-';
\t\tif (ca != cb)
\t\t\treturn false;
\t\tif (!ca)
\t\t\treturn true;
\t}
}

"""

anchor = b"static struct\ndevice_node *regulator_of_get_init_node(struct device *dev,\n"
if anchor not in data:
    sys.exit('FAILED: fungsi regulator_of_get_init_node tidak ketemu')
if b'regu_name_eq' in data:
    sys.exit('FAILED: helper sudah ada')
data = data.replace(anchor, HELPER + anchor, 1)

start = data.index(anchor)
end = data.index(b'struct regulator_init_data *regulator_of_get_init_data', start)
body = data[start:end]
body = body.replace(b'if (!strcmp(desc->of_match, search->name))',
                    b'if (regu_name_eq(desc->of_match, search->name))')
body = body.replace(b'if (!strcmp(desc->of_match, name)) {',
                    b'if (regu_name_eq(desc->of_match, name)) {')
if body.count(b'regu_name_eq(') != 2:
    sys.exit('FAILED: penggantian strcmp tidak lengkap (%d)' % body.count(b'regu_name_eq('))
data = data[:start] + body + data[end:]

open(F, 'wb').write(data)

chk = open(F, 'rb').read()
print('OK: berkas ditulis')
print('  CRLF          :', chk.count(b'\r\n'), '(harus 0)')
print('  LF            :', chk.count(b'\n'))
print('  regu_name_eq  :', chk.count(b'regu_name_eq'))
print('  strcmp of_match:', chk.count(b'strcmp(desc->of_match'))
