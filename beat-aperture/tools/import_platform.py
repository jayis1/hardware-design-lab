#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Reuse the repository author's passive carrier platform, not its experiment."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
source = root.parent / 'catch-witness'
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
    for old, new in [('catch-witness', 'beat-aperture'), ('Catch Witness', 'Beat Aperture'),
                     ('CATCH_', 'BEAT_'), ('CW_', 'BA_'), ('CW1', 'BA1'),
                     ('latch retention witness', 'paired pendulum return'),
                     ('DOOR', 'A_LEFT'), ('BOLT', 'A_RIGHT'),
                     ('HANDLE', 'B_LEFT'), ('TUG', 'B_RIGHT')]:
        text = text.replace(old, new)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text)
print('Imported shared platform; author jayis1')
