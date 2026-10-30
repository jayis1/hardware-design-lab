#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""One-shot provenance-preserving import; refuses to overwrite any output.
Source: punch-register at eab7097850e19c41425e605bf3cfe238e1015d74.
The electrical platform is unchanged; contact meanings and application differ.
"""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
source = root.parent / 'punch-register'
names = ['firmware/main.c', 'firmware/board.h', 'firmware/registers.h',
         'firmware/startup.c', 'firmware/linker.ld', 'firmware/wire.h',
         'firmware/drivers/board.c', 'firmware/drivers/clock.c',
         'firmware/drivers/serial.c', 'firmware/Makefile',
         'tests/test_clock.c', 'tests/test_serial.c', '.gitignore', 'LICENSE',
         'tools/generate_hardware.py', 'tools/check_hardware.py', 'tools/check_quality.py']
for name in names:
    destination = root / name
    if destination.exists():
        raise SystemExit('Refusing existing output: ' + str(destination))
    text = (source / name).read_text()
    for old, new in [('punch-register','maze-return'), ('Punch Register','Maze Return'),
                     ('PUNCH_', 'MAZE_'), ('paper registration witness','magnetic checkpoint recorder'),
                     ('left stop, right stop, bottom, home','checkpoint A, B, C, D'),
                     ("['LEFT', 'RIGHT', 'BOTTOM', 'HOME']", "['GATE_A', 'GATE_B', 'GATE_C', 'GATE_D']")]:
        text = text.replace(old, new)
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(text)
print('Imported', len(names), 'reviewed platform files; author jayis1')
