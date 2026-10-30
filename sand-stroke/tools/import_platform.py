#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Reproduce shared jayis1 platform sources; refuse existing destinations."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT.parent / 'teardown-stack'
FILES = ['firmware/main.c', 'firmware/board.h', 'firmware/Makefile',
         'firmware/registers.h', 'firmware/startup.c', 'firmware/linker.ld',
         'firmware/drivers/board.c', 'firmware/drivers/clock.c',
         'firmware/drivers/serial.c', 'firmware/wire.h',
         'tests/test_clock.c', 'tests/test_serial.c',
         'tools/generate_hardware.py', 'tools/check_hardware.py', 'LICENSE']
for name in FILES:
    text = (SOURCE / name).read_text()
    for old, new in [('teardown-stack', 'sand-stroke'), ('Teardown Stack', 'Sand Stroke'),
                     ('TEARDOWN', 'SAND'), ('cups A, B, C, D', 'left, right, engaged, face B'),
                     ('cup docking interface', 'manual sanding interface'),
                     ("['CUP_A', 'CUP_B', 'CUP_C', 'CUP_D']", "['LEFT', 'RIGHT', 'ENGAGED', 'FACE_B']")]:
        text = text.replace(old, new)
    target = ROOT / name
    if target.exists():
        raise SystemExit(f'Refusing overwrite: {target}')
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text)
    print(target.relative_to(ROOT))
