#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Reuse jayis1's passive carrier and board support from aisle-pair 3b19f2a.
Only platform infrastructure is imported; no measurement interpretation.
"""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
source = root.parent / 'aisle-pair'
names = ['firmware/main.c', 'firmware/board.h', 'firmware/registers.h',
         'firmware/startup.c', 'firmware/linker.ld', 'firmware/drivers/board.c',
         'firmware/drivers/clock.c', 'firmware/drivers/serial.c', 'firmware/Makefile',
         'firmware/wire.h', 'tools/generate_hardware.py', 'tools/check_hardware.py',
         'app/transport.py', 'tests/test_clock.c', 'tests/test_serial.c', 'LICENSE', '.gitignore']
for name in names:
    target = root / name
    if target.exists():
        raise SystemExit('Refusing overwrite: ' + str(target))
    text = (source / name).read_text()
    for old, new in [('aisle-pair', 'sleeve-order'), ('Aisle Pair', 'Sleeve Order'),
                     ('AISLE_BOARD', 'SLEEVE_BOARD'), ('AISLE_WIRE', 'SLEEVE_WIRE'),
                     ('AY1', 'SV1'), ('demo.ay1', 'demo.sv1'),
                     ('opposing drawer threshold', 'telescoping endpoint'),
                     ('A_EARLY', 'OUT_HOME'), ('A_LATE', 'OUT_FAR'),
                     ('B_EARLY', 'IN_HOME'), ('B_LATE', 'IN_FAR')]:
        text = text.replace(old, new)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text)
print('Imported platform infrastructure authored by jayis1')
