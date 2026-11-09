#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Import only shared acquisition and CAD infrastructure from e8a0f67.
The count engine, protocol model, application and experiment tests are new.
Run once; refuse overwrites. Installed footprint licensing is unchanged.
"""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
source = root.parent / 'sector-soak'
names = ['firmware/main.c', 'firmware/board.h', 'firmware/registers.h',
         'firmware/startup.c', 'firmware/linker.ld', 'firmware/drivers/board.c',
         'firmware/drivers/clock.c', 'firmware/drivers/serial.c',
         'firmware/Makefile', 'firmware/wire.h', 'tests/test_clock.c',
         'tests/test_serial.c', '.gitignore', 'LICENSE',
         'tools/generate_hardware.py', 'tools/check_hardware.py', 'app/transport.py']
for name in names:
    destination = root / name
    if destination.exists():
        raise SystemExit('Refusing overwrite: ' + str(destination))
    text = (source / name).read_text()
    for old, new in [('sector-soak', 'tooth-census'), ('Sector Soak', 'Tooth Census'),
                     ('SECTOR_BOARD', 'TOOTH_BOARD'), ('SECTOR_WIRE', 'TOOTH_WIRE'),
                     ('SS1', 'TC1'), ('demo.ss1', 'demo.tc1'),
                     ('Gray-code contact', 'indexed tooth contact'),
                     ('GRAY0', 'ENC_A'), ('GRAY1', 'ENC_B'),
                     ('GRAY2', 'INDEX'), ('GRAY3', 'TOOTH')]:
        text = text.replace(old, new)
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(text)
print('Imported shared platform; no device engine or model reused')
