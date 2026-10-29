#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""_mk_transkrip.py -- kumpulkan semua baris yang dicetak ke layar oleh init.c.

Menghasilkan out/transkrip_referensi.txt: urutan pesan VXTUX-* apa adanya dari
sumber, lengkap dengan nomor baris.  Berguna untuk membandingkan dengan
00-transcript.log hasil boot sungguhan (file itu ditulis oleh init di tablet).
"""
import io
import re

SRC = 'ci_qemu/initramfs/init.c'
OUT = 'out/transkrip_referensi.txt'

CALL = re.compile(
    r'\b(?:both|both_fmt|con|con_fmt)\(\s*((?:"(?:[^"\\]|\\.)*"\s*)+)')
LIT = re.compile(r'"((?:[^"\\]|\\.)*)"')


def main():
    src = io.open(SRC, encoding='utf-8').read()
    rows = []
    for m in CALL.finditer(src):
        line_no = src.count('\n', 0, m.start()) + 1
        text = ''.join(LIT.findall(m.group(1)))
        text = text.encode('latin-1', 'backslashreplace').decode('unicode_escape')
        if text.strip():
            rows.append((line_no, text))

    with io.open(OUT, 'w', encoding='utf-8', newline='\n') as f:
        f.write('# Transkrip REFERENSI: urutan pesan layar menurut init.c\n')
        f.write('# Bukan rekaman boot. Rekaman sungguhan ada di perangkat:\n')
        f.write('#   /cache/vxtux-logs/00-transcript.log\n')
        f.write('#   (dan salinannya di kanal mentah common_rs1_a)\n\n')
        for ln, text in rows:
            for part in text.split('\n'):
                if part.strip():
                    f.write('init.c:%-5d | %s\n' % (ln, part))
    print('pemanggilan layar: %d -> %s' % (len(rows), OUT))


if __name__ == '__main__':
    main()
