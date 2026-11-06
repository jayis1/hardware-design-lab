#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Import shared jayis1 board support from pallet-census, reviewed at 182df31.
The pump engine and application model are not imported. Refuse overwrites.
"""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
source = root.parent / 'pallet-census'
names = ['firmware/main.c', 'firmware/board.h', 'firmware/registers.h',
         'firmware/startup.c', 'firmware/linker.ld', 'firmware/drivers/board.c',
         'firmware/drivers/clock.c', 'firmware/drivers/serial.c', 'firmware/Makefile',
         'firmware/wire.h', 'tests/test_clock.c', 'tests/test_serial.c', '.gitignore',
         'LICENSE', 'tools/generate_hardware.py', 'tools/check_hardware.py',
         'app/transport.py', 'tools/verify.py']
for name in names:
    target = root / name
    if target.exists():
        raise SystemExit('Refusing overwrite: ' + str(target))
    text = (source / name).read_text()
    for old, new in [('pallet-census', 'stroke-weave'), ('Pallet Census', 'Stroke Weave'),
                     ('PALLET_BOARD', 'STROKE_BOARD'), ('PALLET_WIRE', 'STROKE_WIRE'),
                     ('PC1', 'TW1'), ('demo.pc1', 'demo.sw1'),
                     ('escapement contact', 'twin-piston endpoint'),
                     ('PALLET_A', 'A_HOME'), ('PALLET_B', 'A_END'),
                     ('INDEX', 'B_HOME'), ('RUN', 'B_END')]:
        text = text.replace(old, new)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text)
print('Imported shared jayis1 platform; new engine required')
