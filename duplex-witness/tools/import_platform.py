#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Recreate shared jayis1 platform files from kit-multiset (dbf6043).
Measurement engine, wire encoder, model and UI are not imported.
Existing files are never overwritten. Run from this repository only.
"""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
source = root.parent / 'kit-multiset'
names = ['firmware/main.c', 'firmware/board.h', 'firmware/registers.h',
         'firmware/startup.c', 'firmware/linker.ld', 'firmware/wire.h',
         'firmware/drivers/board.c', 'firmware/drivers/clock.c',
         'firmware/drivers/serial.c', 'firmware/Makefile',
         'tests/test_clock.c', 'tests/test_serial.c', '.gitignore', 'LICENSE',
         'tools/generate_hardware.py', 'tools/check_hardware.py', 'app/transport.py']
for name in names:
    target = root / name
    if target.exists():
        raise SystemExit('Refusing overwrite: ' + str(target))
    text = (source / name).read_text()
    for old, new in [('kit-multiset', 'duplex-witness'), ('Kit Multiset', 'Duplex Witness'),
                     ('KIT_', 'DUPLEX_'), ('unordered kitting stroke recorder', 'two-face stamp witness'),
                     ('bin A, bin B, bin C, tray', 'face A, face B, press, captive latch'),
                     ("['BIN_A', 'BIN_B', 'BIN_C', 'TRAY']", "['FACE_A', 'FACE_B', 'PRESS', 'LATCH']"),
                     ('KM1', 'DW1')]:
        text = text.replace(old, new)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text)
print('Imported shared platform; author jayis1')
