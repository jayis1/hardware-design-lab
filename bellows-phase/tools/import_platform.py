#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Reuse jayis1 platform from lay-count at 14a3ee6, never overwrite."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
source = root.parent / 'lay-count'
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
    for old, new in [('lay-count', 'bellows-phase'), ('Lay Count', 'Bellows Phase'),
                     ('LAY_', 'BELLOWS_'), ('manual winding traverse recorder', 'manual bellows valve recorder'),
                     ('left, right, spool index, tension indication', 'compressed, expanded, inlet open, outlet open'),
                     ("['LEFT', 'RIGHT', 'INDEX', 'TENSION']", "['COMPRESSED', 'EXPANDED', 'INLET', 'OUTLET']"),
                     ('LC1', 'BP1')]:
        text = text.replace(old, new)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text)
print('Imported platform and passive circuit; author jayis1')
