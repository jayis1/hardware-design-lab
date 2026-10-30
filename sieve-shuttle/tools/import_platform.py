#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Import reviewed jayis1 platform sources, without overwriting outputs.
Source: maze-return at 5fd0821439c6e3fd5615900e7374021a3b961d0f.
The passive circuit and STM32 target remain unchanged, not the experiment.
"""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
source = root.parent / 'maze-return'
names = ['firmware/main.c', 'firmware/board.h', 'firmware/registers.h',
         'firmware/startup.c', 'firmware/linker.ld', 'firmware/wire.h',
         'firmware/drivers/board.c', 'firmware/drivers/clock.c',
         'firmware/drivers/serial.c', 'firmware/Makefile',
         'tests/test_clock.c', 'tests/test_serial.c', '.gitignore', 'LICENSE',
         'tools/generate_hardware.py', 'tools/check_hardware.py']
for name in names:
    destination = root / name
    if destination.exists():
        raise SystemExit('Refusing existing output: ' + str(destination))
    text = (source / name).read_text()
    for old, new in [('maze-return','sieve-shuttle'), ('Maze Return','Sieve Shuttle'),
                     ('MAZE_', 'SIEVE_'), ('magnetic checkpoint recorder','manual sieve motion recorder'),
                     ('checkpoint A, B, C, D','west, east, north, south zones'),
                     ("['GATE_A', 'GATE_B', 'GATE_C', 'GATE_D']", "['WEST', 'EAST', 'NORTH', 'SOUTH']")]:
        text = text.replace(old, new)
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(text)
print('Imported reviewed platform sources; author jayis1')
