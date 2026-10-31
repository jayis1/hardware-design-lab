#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""One-time import of jayis1 platform from coupon-transit at 4d5cc29.
Measurement engine and application model are not imported. Refuse overwrites.
"""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
source = root.parent / 'coupon-transit'
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
    for old, new in [('coupon-transit', 'pattern-sled'), ('Coupon Transit', 'Pattern Sled'),
                     ('TRANSIT_', 'PATTERN_'), ('paired holder transit recorder', 'pattern repeat comparator'),
                     ('A source, A destination, B source, B destination', 'clock, data 0, data 1, data 2'),
                     ("['A_SRC', 'A_DST', 'B_SRC', 'B_DST']", "['CLOCK', 'DATA0', 'DATA1', 'DATA2']"),
                     ('CT1', 'PS1')]:
        text = text.replace(old, new)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text)
print('Imported platform; author jayis1')
