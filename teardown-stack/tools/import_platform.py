#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Import shared, authored platform only; no measurement or app code."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT.parent / 'flow-pause'
FILES = ['firmware/main.c', 'firmware/board.h', 'firmware/Makefile',
         'firmware/registers.h', 'firmware/startup.c', 'firmware/linker.ld',
         'firmware/drivers/board.c', 'firmware/drivers/clock.c',
         'firmware/drivers/serial.c', 'tests/test_clock.c', 'tests/test_serial.c',
         'tools/generate_hardware.py', 'tools/check_hardware.py', 'LICENSE']
for name in FILES:
    text = (SOURCE / name).read_text()
    for old, new in [('flow-pause', 'teardown-stack'), ('Flow Pause', 'Teardown Stack'),
                     ('FLOW_BOARD_H', 'TEARDOWN_BOARD_H'),
                     ('discharge, supply, intervention, enable', 'cups A, B, C, D'),
                     ('hopper observation interface', 'cup docking interface'),
                     ("['DISCHARGE', 'SUPPLY', 'INTERVENE', 'ENABLE']",
                      "['CUP_A', 'CUP_B', 'CUP_C', 'CUP_D']")]:
        text = text.replace(old, new)
    target = ROOT / name
    if target.exists():
        raise SystemExit(f'Refusing overwrite: {target}')
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text)
    print(target.relative_to(ROOT))
