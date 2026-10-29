#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
bldb-cli -- Berry Lab Foundation command line client untuk BLDB.

License: Berry Foundation Public v1 - MIT
Independent creation; tidak memakai kode Unisoc/Spreadtrum.  Berbicara
protokol teks BLDB lewat port serial USB (gadget ACM dari tablet).

Perangkat tablet menampilkan identitas palsu Berry saat fase initramfs:
    VID:PID 1209:b1db   serial 0123456789ABCDEF
(adb tidak akan menyentuhnya -- justru itu gunanya pintu kedua ini.)

Pemakaian:
    python bldb_cli.py devices                 # daftar port kandidat
    python bldb_cli.py log COM3                # streaming log live (Ctrl+C utk stop)
    python bldb_cli.py cmd COM3 stats          # kirim satu perintah, baca balasan
    python bldb_cli.py cmd COM3 "poke32 9d000000 deadbeef"
    python bldb_cli.py save COM3 keluaran.txt  # rekam stream ke file

Perintah yang dikenal kernel (tulis lewat `cmd`):
    help | stats | clear | mark <teks>
    peek32 <hex>            baca RAM (hanya RAM valid, ditolak di luar RAM)
    poke32 <hex> <hex>      tulis RAM (ditolak di teks kernel & ring)
"""
import sys
import time

try:
    import serial  # pyserial
    from serial.tools import list_ports
except ImportError:
    serial = None
    list_ports = None

BAUD = 115200  # ACM tidak peduli baud; nilainya hanya formalitas


def die(msg, code=1):
    sys.stderr.write("bldb-cli: %s\n" % msg)
    sys.exit(code)


def cmd_devices():
    if list_ports is None:
        die("pyserial belum terpasang: pip install pyserial")
    found = 0
    print("List of BLDB devices attached:")
    for p in list_ports.comports():
        desc = (p.description or "?")
        hwid = (p.hwid or "")
        vid = getattr(p, "vid", None)
        pid = getattr(p, "pid", None)
        tag = ""
        if vid == 0x1209 and pid == 0xB1DB:
            tag = "  <== BLDB (fase initramfs)"
        print("  %-14s %-40s vid:pid=%s serial=%s%s" % (
            p.device, desc[:40],
            ("%04x:%04x" % (vid, pid)) if vid else hwid,
            p.serial_number or "?", tag))
        found += 1
    if not found:
        print("  (tidak ada port serial; tablet sudah boot ke gadget ACM belum?)")
    return 0


def open_port(dev, timeout=2.0):
    if serial is None:
        die("pyserial belum terpasang: pip install pyserial")
    return serial.Serial(dev, BAUD, timeout=timeout)


def drain(ser, seconds=None, outfile=None, until=None):
    """Baca selama `seconds` (None = selamanya) atau sampai `until` (bytes)."""
    start = time.time()
    acc = b""
    while True:
        if seconds is not None and time.time() - start > seconds:
            break
        chunk = ser.read(4096)
        if chunk:
            acc += chunk
            sys.stdout.write(chunk.decode("utf-8", "replace"))
            sys.stdout.flush()
            if outfile:
                outfile.write(chunk)
                outfile.flush()
            if until and until in acc:
                break
        elif seconds is None and outfile is None and not until:
            # mode stream tanpa file: terus selamanya sampai Ctrl+C
            continue
    return acc


def cmd_log(dev, outfile_path=None):
    ser = open_port(dev, timeout=1.0)
    print("[bldb-cli] streaming dari %s (Ctrl+C untuk berhenti)..." % dev)
    fo = open(outfile_path, "wb") if outfile_path else None
    try:
        if fo:
            # rekam sampai dihentikan
            while True:
                chunk = ser.read(4096)
                if chunk:
                    fo.write(chunk)
                    fo.flush()
                    sys.stdout.write(chunk.decode("utf-8", "replace"))
                    sys.stdout.flush()
        else:
            while True:
                chunk = ser.read(4096)
                if chunk:
                    sys.stdout.write(chunk.decode("utf-8", "replace"))
                    sys.stdout.flush()
    except KeyboardInterrupt:
        print("\n[bldb-cli] berhenti.")
    finally:
        if fo:
            fo.close()
        ser.close()
    return 0


def cmd_cmd(dev, command, wait=1.5):
    ser = open_port(dev, timeout=0.5)
    # bersihkan sisa antrian lama
    ser.reset_input_buffer()
    ser.write(command.encode() + b"\n")
    ser.flush()
    print("[bldb-cli] > %s" % command)
    drain(ser, seconds=wait)
    ser.close()
    return 0


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 1
    sub = argv[1]
    if sub == "devices":
        return cmd_devices()
    if sub in ("log", "save"):
        if len(argv) < 3:
            die("pakai: bldb-cli %s <PORT> [file]" % sub)
        out = argv[3] if (sub == "save" and len(argv) > 3) else None
        return cmd_log(argv[2], out)
    if sub == "cmd":
        if len(argv) < 4:
            die("pakai: bldb-cli cmd <PORT> <perintah>")
        return cmd_cmd(argv[2], " ".join(argv[3:]))
    print(__doc__)
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
