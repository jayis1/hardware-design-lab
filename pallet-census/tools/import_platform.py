#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Import reviewed passive-carrier support; does not import another device engine."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
source = root.parent / 'key-escrow'
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
    for old, new in [('key-escrow', 'pallet-census'), ('Key Escrow', 'Pallet Census'),
                     ('ESCROW', 'PALLET'), ('KE1', 'PC1'), ('demo.ke1', 'demo.pc1'),
                     ('key sequence contact', 'escapement contact'),
                     ('RETRACTED', 'PALLET_A'), ('EXTENDED', 'PALLET_B'),
                     ('KEY_SEATED', 'INDEX'), ('FLAP_CLOSED', 'RUN')]:
        text = text.replace(old, new)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text)
print('Imported reviewed jayis1 platform')
