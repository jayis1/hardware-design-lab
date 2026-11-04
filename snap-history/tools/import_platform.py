#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Import jayis1's existing passive carrier; refuse overwriting any file."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
source = root.parent / 'grip-lattice'
names = ['firmware/main.c', 'firmware/board.h', 'firmware/registers.h',
         'firmware/startup.c', 'firmware/linker.ld', 'firmware/drivers/board.c',
         'firmware/drivers/clock.c', 'firmware/drivers/serial.c', 'firmware/Makefile',
         'tests/test_clock.c', 'tests/test_serial.c', '.gitignore', 'LICENSE',
         'tools/generate_hardware.py', 'tools/check_hardware.py', 'app/transport.py']
for name in names:
    destination = root / name
    if destination.exists():
        raise SystemExit('Refusing overwrite: ' + str(destination))
    text = (source / name).read_text()
    for old, new in [('grip-lattice', 'snap-history'), ('Grip Lattice', 'Snap History'),
                     ('GRIP_', 'SNAP_'), ('GL_', 'SH_'), ('GL1', 'SH1'),
                     ('grip contact', 'fastener contact'),
                     ('THUMB', 'SNAP_A'), ('FINGERS', 'SNAP_B'),
                     ('PALM', 'SNAP_C'), ('RECORD', 'SNAP_D')]:
        text = text.replace(old, new)
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(text)
print('Imported shared platform authored by jayis1')
