#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Reproduce explicitly shared jayis1 platform sources from the sibling design.
Run only when intentionally refreshing the platform; inspect resulting diffs.
Measurement engines and applications are not imported.
"""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT.parent / 'rest-quartet'
FILES = ['firmware/main.c', 'firmware/board.h', 'firmware/Makefile',
         'firmware/registers.h', 'firmware/startup.c', 'firmware/linker.ld',
         'firmware/drivers/board.c', 'firmware/drivers/clock.c',
         'firmware/drivers/serial.c', 'tests/test_clock.c', 'tests/test_serial.c',
         'tools/generate_hardware.py', 'tools/check_hardware.py', 'LICENSE']
for name in FILES:
    text = (SOURCE / name).read_text()
    for old, new in [('rest-quartet', 'flow-pause'), ('Rest Quartet', 'Flow Pause'),
                     ('REST_BOARD_H', 'FLOW_BOARD_H'),
                     ('pockets A..D', 'discharge, supply, intervention, enable'),
                     ('pocket interface', 'hopper observation interface'),
                     ("['POCKET_A', 'POCKET_B', 'POCKET_C', 'POCKET_D']",
                      "['DISCHARGE', 'SUPPLY', 'INTERVENE', 'ENABLE']")]:
        text = text.replace(old, new)
    target = ROOT / name
    if target.exists():
        raise SystemExit(f'Refusing overwrite: {target}')
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text)
    print(target.relative_to(ROOT))
