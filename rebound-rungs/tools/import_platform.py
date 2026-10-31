#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Import the existing jayis1 F401 platform; never overwrite local files.
Source revision: pattern-sled at 8452cdb. No measurement engine is copied.
"""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
source = root.parent / 'pattern-sled'
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
    for old, new in [('pattern-sled', 'rebound-rungs'), ('Pattern Sled', 'Rebound Rungs'),
                     ('PATTERN_', 'REBOUND_'), ('pattern repeat comparator', 'first-rebound gate recorder'),
                     ('clock, data 0, data 1, data 2', 'rung 0, rung 1, rung 2, rung 3'),
                     ("['CLOCK', 'DATA0', 'DATA1', 'DATA2']", "['RUNG0', 'RUNG1', 'RUNG2', 'RUNG3']"),
                     ('PS1', 'RR1')]:
        text = text.replace(old, new)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text)
print('Imported platform; author jayis1')
