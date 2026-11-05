#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Import the inspected jayis1 F401 passive carrier platform without overwrite."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
source = root.parent / 'detent-route'
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
    for old, new in [('detent-route', 'toggle-hold'), ('Detent Route', 'Toggle Hold'),
                     ('STATION_1OARD_H', 'TOGGLE_BOARD_H'), ('DR1', 'TH1'),
                     ('demo.dr1', 'demo.th1'),
                     ('indexed station contact', 'bistable retention contact'),
                     ('STATION_0', 'END_A'), ('STATION_1', 'END_B'),
                     ('STATION_2', 'PUSH_B'), ('STATION_3', 'PUSH_A')]:
        text = text.replace(old, new)
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(text)
print('Imported inspected platform authored by jayis1')
