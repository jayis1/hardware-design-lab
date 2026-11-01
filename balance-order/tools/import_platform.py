#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Import jayis1's reviewed F401/passive carrier platform, not its experiment."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
source = root.parent / 'braid-trace'
names = ['firmware/main.c', 'firmware/board.h', 'firmware/registers.h',
         'firmware/startup.c', 'firmware/linker.ld', 'firmware/wire.h',
         'firmware/drivers/board.c', 'firmware/drivers/clock.c',
         'firmware/drivers/serial.c', 'firmware/Makefile', 'firmware/wire.c',
         'tests/test_clock.c', 'tests/test_serial.c', '.gitignore', 'LICENSE',
         'tools/generate_hardware.py', 'tools/check_hardware.py', 'app/transport.py']
for name in names:
    target = root / name
    if target.exists():
        raise SystemExit('Refusing overwrite: ' + str(target))
    text = (source / name).read_text()
    for old, new in [('braid-trace', 'balance-order'), ('Braid Trace', 'Balance Order'),
                     ('BRAID_', 'BALANCE_'), ('BT_', 'BO_'), ('BT1', 'BO1'),
                     ('indexed crossing guide', 'deadband balance comparator'),
                     ('PAIR_LEFT', 'PAIR0'), ('PAIR_RIGHT', 'PAIR1'),
                     ('OVER', 'LEFT_LOW'), ('UNDER', 'RIGHT_LOW'),
                     ('left detent, right detent', 'pair bit 0, pair bit 1')]:
        text = text.replace(old, new)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text)
print('Imported shared platform; author jayis1')
