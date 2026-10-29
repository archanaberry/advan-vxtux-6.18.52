import io
import os
import shutil

# 1. pipeline artix42 = salinan artix41 dengan path baru -------------------
src = 'out/ci_artix41_pipeline.sh'
dst = 'out/ci_artix42_pipeline.sh'
s = io.open(src, newline='').read()
s = s.replace('ci_artix41_pipeline.sh', 'ci_artix42_pipeline.sh')
s = s.replace('out/ci_artix41_src', 'out/ci_artix42_src')
s = s.replace('out/dtb_artix41', 'out/dtb_artix42')
s = s.replace('boot_artix41', 'boot_artix42')
s = s.replace('artix41', 'artix42')
io.open(dst, 'w', newline='').write(s)
print('OK pipeline :', dst, '(%d byte)' % os.path.getsize(dst))
print('   rujukan artix41 tersisa :', s.count('artix41'))

# dtb sumber disalin apa adanya (DTS tidak berubah)
os.makedirs('out/dtb_artix42', exist_ok=True)
shutil.copy2('out/dtb_artix41/ums512-1h10-vxtux.dtb', 'out/dtb_artix42/')
print('OK dtb      : out/dtb_artix42/ums512-1h10-vxtux.dtb')

# 2. skrip vendor_boot pindah folder log ke logs/vb42 ----------------------
for f in ('out/dump_dt42.bat', 'out/build_vendor42.bat',
          'out/flash_vendor42.bat', 'out/restore_vendor42.bat'):
    t = io.open(f, newline='').read()
    n = t.replace(r'logs\artix42', r'logs\vb42')
    if n == t:
        print('[!] tidak berubah:', f)
        continue
    io.open(f, 'w', newline='').write(n)
    print('OK log dir  :', f, '-> logs\\vb42')
