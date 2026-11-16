#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Import explicitly shared infrastructure from bin-phrase at df23325.
No measurement engine, record interpretation or GUI is imported.
"""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
source = root.parent / 'bin-phrase'
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
    for old, new in [('bin-phrase', 'aisle-pair'), ('Bin Phrase', 'Aisle Pair'),
                     ('PHRASE_BOARD', 'AISLE_BOARD'), ('PHRASE_WIRE', 'AISLE_WIRE'),
                     ('BQ1', 'AY1'), ('demo.bq1', 'demo.ay1'),
                     ('bin opening contact', 'opposing drawer threshold'),
                     ('OPEN_A', 'A_EARLY'), ('OPEN_B', 'A_LATE'),
                     ('OPEN_C', 'B_EARLY'), ('TRAY', 'B_LATE')]:
        text = text.replace(old, new)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text)
print('Imported shared infrastructure credited to jayis1')
