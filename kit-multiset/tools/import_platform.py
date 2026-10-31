#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Import jayis1's common passive F401 platform from rebound-rungs at 52a3bd1.
No measurement engine, protocol encoder or application model is imported.
Run only to recreate missing platform files; existing files are never replaced.
"""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
source = root.parent / 'rebound-rungs'
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
    for old, new in [('rebound-rungs', 'kit-multiset'), ('Rebound Rungs', 'Kit Multiset'),
                     ('REBOUND_', 'KIT_'), ('first-rebound gate recorder', 'unordered kitting stroke recorder'),
                     ('rung 0, rung 1, rung 2, rung 3', 'bin A, bin B, bin C, tray'),
                     ("['RUNG0', 'RUNG1', 'RUNG2', 'RUNG3']", "['BIN_A', 'BIN_B', 'BIN_C', 'TRAY']"),
                     ('RR1', 'KM1')]:
        text = text.replace(old, new)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text)
print('Imported platform; author jayis1')
