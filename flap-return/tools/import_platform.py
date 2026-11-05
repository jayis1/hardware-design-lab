#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Import the existing authored F401 passive platform without overwriting."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
source = root.parent / 'snap-history'
names = ['firmware/main.c', 'firmware/board.h', 'firmware/registers.h',
         'firmware/startup.c', 'firmware/linker.ld', 'firmware/drivers/board.c',
         'firmware/drivers/clock.c', 'firmware/drivers/serial.c', 'firmware/Makefile',
         'tests/test_clock.c', 'tests/test_serial.c', '.gitignore', 'LICENSE',
         'tools/generate_hardware.py', 'tools/check_hardware.py', 'app/transport.py',
         'firmware/wire.h']
for name in names:
    destination = root / name
    if destination.exists():
        raise SystemExit('Refusing overwrite: ' + str(destination))
    text = (source / name).read_text()
    for old, new in [('snap-history', 'flap-return'), ('Snap History', 'Flap Return'),
                     ('SNAP_', 'FLAP_'), ('SH1', 'FR1'),
                     ('fastener contact', 'flap return contact'),
                     ('FLAP_D', 'RELEASE_HOLD')]:
        text = text.replace(old, new)
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(text)
print('Imported shared platform authored by jayis1')
