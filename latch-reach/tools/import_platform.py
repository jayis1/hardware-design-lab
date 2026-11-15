#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Import inspected acquisition/carrier infrastructure from a52abd2.
No experiment engine or application interpretation is imported.
"""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
source = root.parent / 'float-band'
names = ['firmware/main.c', 'firmware/board.h', 'firmware/registers.h',
         'firmware/startup.c', 'firmware/linker.ld', 'firmware/drivers/board.c',
         'firmware/drivers/clock.c', 'firmware/drivers/serial.c',
         'firmware/Makefile', 'firmware/wire.h', 'tools/generate_hardware.py',
         'tools/check_hardware.py', 'app/transport.py',
         'tests/test_clock.c', 'tests/test_serial.c', 'LICENSE', '.gitignore']
for name in names:
    target = root / name
    if target.exists():
        raise SystemExit('Refusing overwrite: ' + str(target))
    text = (source / name).read_text()
    for old, new in [('float-band', 'latch-reach'), ('Float Band', 'Latch Reach'),
                     ('FLOAT_BOARD', 'REACH_BOARD'), ('FLOAT_WIRE', 'REACH_WIRE'),
                     ('FB1', 'LR1'), ('demo.fb1', 'demo.lr1'),
                     ('reference-level contact', 'opening-checkpoint contact')]:
        text = text.replace(old, new)
    if name in ('firmware/board.h', 'tools/generate_hardware.py'):
        for old, new in [('LOW', 'HOME'), ('MID', 'NEAR'), ('HIGH', 'FAR'), ('DUT', 'LATCH')]:
            text = text.replace(old, new)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text)
print('Imported shared platform only; author jayis1')
