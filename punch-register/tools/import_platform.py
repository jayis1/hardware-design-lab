#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Import the repository's reviewed passive platform without overwriting."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT.parent / 'merge-rhythm'
FILES = ['firmware/main.c', 'firmware/board.h', 'firmware/Makefile',
         'firmware/registers.h', 'firmware/startup.c', 'firmware/linker.ld',
         'firmware/drivers/board.c', 'firmware/drivers/clock.c',
         'firmware/drivers/serial.c', 'tests/test_clock.c', 'tests/test_serial.c',
         'tools/generate_hardware.py', 'tools/check_hardware.py', 'LICENSE']
for name in FILES:
    text = (SOURCE / name).read_text()
    for old, new in [('merge-rhythm', 'punch-register'), ('Merge Rhythm', 'Punch Register'),
                     ('MERGE_BOARD', 'PUNCH_BOARD'),
                     ('left, right, exit, clear', 'left stop, right stop, bottom, home'),
                     ('chute interleaving interface', 'paper registration witness'),
                     ("['LEFT', 'RIGHT', 'EXIT', 'CLEAR']", "['LEFT', 'RIGHT', 'BOTTOM', 'HOME']")]:
        text = text.replace(old, new)
    target = ROOT / name
    if target.exists():
        raise SystemExit(f'Refusing overwrite: {target}')
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text)
    print(target.relative_to(ROOT))
