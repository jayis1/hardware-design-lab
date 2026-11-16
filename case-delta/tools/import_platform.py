#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Import reviewed infrastructure from Latch Reach at a68dadc.
The experiment engine and application interpretation are not imported.
Run once only; existing destinations are never overwritten.
"""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
source = root.parent / 'latch-reach'
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
    for old, new in [('latch-reach', 'case-delta'), ('Latch Reach', 'Case Delta'),
                     ('REACH_BOARD', 'CASE_BOARD'), ('REACH_WIRE', 'CASE_WIRE'),
                     ('LR1', 'CD1'), ('demo.lr1', 'demo.cd1'),
                     ('opening-checkpoint contact', 'case inventory contact')]:
        text = text.replace(old, new)
    if name in ('firmware/board.h', 'tools/generate_hardware.py'):
        for old, new in [('HOME', 'POCKET_A'), ('NEAR', 'POCKET_B'),
                         ('FAR', 'POCKET_C'), ('LATCH', 'LID')]:
            text = text.replace(old, new)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text)
print('Imported reviewed jayis1 platform infrastructure')
