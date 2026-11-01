#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Import only jayis1's shared F401 carrier platform, not its experiment."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
source = root.parent / 'balance-order'
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
    for old, new in [('balance-order', 'catch-witness'), ('Balance Order', 'Catch Witness'),
                     ('BALANCE_', 'CATCH_'), ('BO_', 'CW_'), ('BO1', 'CW1'),
                     ('deadband balance comparator', 'latch retention witness'),
                     ('PAIR0', 'DOOR'), ('PAIR1', 'BOLT'),
                     ('LEFT_LOW', 'HANDLE'), ('RIGHT_LOW', 'TUG'),
                     ('pair bit 0, pair bit 1', 'DOOR, BOLT')]:
        text = text.replace(old, new)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text)
print('Imported shared platform; author jayis1')
