#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Import reviewed jayis1 platform sources; never overwrite existing files."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT.parent / 'sand-stroke'
FILES = ['firmware/main.c', 'firmware/board.h', 'firmware/Makefile',
         'firmware/registers.h', 'firmware/startup.c', 'firmware/linker.ld',
         'firmware/drivers/board.c', 'firmware/drivers/clock.c',
         'firmware/drivers/serial.c', 'tests/test_clock.c', 'tests/test_serial.c',
         'tools/generate_hardware.py', 'tools/check_hardware.py', 'LICENSE']
for name in FILES:
    text = (SOURCE / name).read_text()
    for old, new in [('sand-stroke', 'merge-rhythm'), ('Sand Stroke', 'Merge Rhythm'),
                     ('SAND_BOARD', 'MERGE_BOARD'),
                     ('left, right, engaged, face B', 'left, right, exit, clear'),
                     ('manual sanding interface', 'chute interleaving interface'),
                     ("['LEFT', 'RIGHT', 'ENGAGED', 'FACE_B']", "['LEFT', 'RIGHT', 'EXIT', 'CLEAR']")]:
        text = text.replace(old, new)
    target = ROOT / name
    if target.exists():
        raise SystemExit(f'Refusing overwrite: {target}')
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text)
    print(target.relative_to(ROOT))
