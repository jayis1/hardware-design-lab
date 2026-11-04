#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""One-time import of jayis1's shared carrier and MCU platform."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
source = root.parent / 'beat-aperture'
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
    for old, new in [('beat-aperture', 'domino-ledger'), ('Beat Aperture', 'Domino Ledger'),
                     ('BEAT_', 'DOMINO_'), ('BA_', 'DL_'), ('BA1', 'DL1'),
                     ('paired pendulum return', 'hinged cascade'),
                     ('A_LEFT', 'UP_A'), ('A_RIGHT', 'UP_B'),
                     ('B_LEFT', 'UP_C'), ('B_RIGHT', 'UP_D')]:
        text = text.replace(old, new)
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(text)
print('Imported shared platform; author jayis1')
