import io, re, sys

files = [
    "dump_artix37_full.bat",
    "dump_artix40_logonly.bat",
    "dump_flash_artix40.bat",
    "dump_artix41_logonly.bat",
    "dump_flash_artix41.bat",
    "flash_artix41.bat",
    "restore_dtb41.bat",
]

bad = 0
for f in files:
    s = io.open(f, newline='').read()
    lines = s.split('\r\n')
    pushd = sum(1 for l in lines if l.strip().lower() == 'pushd "%log%"')
    popd = sum(1 for l in lines if l.strip().lower() == 'popd')
    pathcmd = [i + 1 for i, l in enumerate(lines) if l.strip().startswith('path "')]
    devnull = [i + 1 for i, l in enumerate(lines) if '/dev/null' in l]
    outuse = '%OUT%' in s
    outset = ('set OUT=%~dp0' in s)
    problem = []
    if pushd != popd:
        problem.append("pushd=%d popd=%d" % (pushd, popd))
    if pathcmd:
        problem.append("masih ada 'path \"' di baris %s" % pathcmd)
    if devnull:
        problem.append("/dev/null di baris %s" % devnull)
    if outuse and not outset:
        problem.append("%OUT% dipakai tapi tidak di-set")
    if problem:
        bad += 1
        print("[X] %-26s %s" % (f, "; ".join(problem)))
    else:
        print("[OK] %-26s pushd/popd=%d/%d" % (f, pushd, popd))

# pastikan pushd muncul SEBELUM spd_dump dan popd SESUDAH
for f in files:
    s = io.open(f, newline='').read()
    i_spd = s.find('"%SPD%" --wait')
    if i_spd < 0:
        continue
    i_push = s.find('pushd "%LOG%"')
    i_reset = s.find('reset', i_spd)
    i_pop = s.find('\r\npopd', i_reset)
    ok = (i_push != -1 and i_push < i_spd and i_pop != -1)
    print(("  [OK] urutan pushd->spd_dump->popd : %s" if ok else "  [X] urutan SALAH : %s") % f)
    if not ok:
        bad += 1

sys.exit(1 if bad else 0)
