#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Import the inspected, reusable jayis1 Nucleo passive-carrier platform once."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
source = root.parent / 'vacancy-walk'
names = ['firmware/main.c', 'firmware/board.h', 'firmware/registers.h',
         'firmware/startup.c', 'firmware/linker.ld', 'firmware/drivers/board.c',
         'firmware/drivers/clock.c', 'firmware/drivers/serial.c', 'firmware/Makefile',
         'firmware/wire.h', 'tests/test_clock.c', 'tests/test_serial.c', '.gitignore',
         'LICENSE', 'tools/generate_hardware.py', 'tools/check_hardware.py', 'app/transport.py']
for name in names:
    target = root / name
    if target.exists():
        raise SystemExit('Refusing overwrite: ' + str(target))
    text = (source / name).read_text()
    for old, new in [('vacancy-walk', 'key-escrow'), ('Vacancy Walk', 'Key Escrow'),
                     ('VACANCY', 'ESCROW'), ('VW1', 'KE1'), ('demo.vw1', 'demo.ke1'),
                     ('pocket occupancy contact', 'key sequence contact'),
                     ('POCKET_0', 'RETRACTED'), ('POCKET_1', 'EXTENDED'),
                     ('POCKET_2', 'KEY_SEATED'), ('POCKET_3', 'FLAP_CLOSED')]:
        text = text.replace(old, new)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text)
print('Imported inspected platform; author jayis1')
