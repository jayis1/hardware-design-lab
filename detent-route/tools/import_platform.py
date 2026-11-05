#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Copy the inspected, jayis1-authored F401 carrier platform, never overwrite."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
source = root.parent / 'flap-return'
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
    for old, new in [('flap-return', 'detent-route'), ('Flap Return', 'Detent Route'),
                     ('FLAP_', 'DETENT_'), ('FR1', 'DR1'),
                     ('flap return contact', 'indexed station contact'),
                     ('DETENT_A', 'STATION_0'), ('DETENT_B', 'STATION_1'),
                     ('DETENT_C', 'STATION_2'), ('RELEASE_HOLD', 'STATION_3')]:
        text = text.replace(old, new)
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(text)
print('Imported inspected platform authored by jayis1')
