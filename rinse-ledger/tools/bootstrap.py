#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Reuse jayis1's established board support and passive carrier topology.
Only creates missing files; never overwrites existing work. Run from this repo.
"""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT.parent / 'pedal-window'
paths = ['firmware/startup.c', 'firmware/linker.ld', 'firmware/registers.h',
         'firmware/drivers/board.c', 'firmware/drivers/serial.c', 'firmware/board.h',
         'firmware/Makefile', 'tools/generate_hardware.py', 'tools/check_hardware.py',
         'tools/verify.py', '.gitignore']
for name in paths:
    text = (BASE / name).read_text()
    text = text.replace('pedal-window', 'rinse-ledger').replace('Pedal Window', 'Rinse Ledger')
    text = text.replace('piano-contact', 'basket-contact').replace('pedal-eda-', 'rinse-eda-')
    text = text.replace('PEDAL_', 'RINSE_').replace('Pedal', 'Rinse')
    text = text.replace("['KEY_A', 'KEY_B', 'PEDAL_REST', 'PEDAL_DOWN']", "['BASKET_A', 'BASKET_B', 'BASKET_C', 'WATER_CHANGE']")
    # The identifier substitution above also affects quoted signal names.
    text = text.replace("['KEY_A', 'KEY_B', 'RINSE_REST', 'RINSE_DOWN']", "['BASKET_A', 'BASKET_B', 'BASKET_C', 'WATER_CHANGE']")
    text = text.replace('256u', '512u').replace('J3 key A, J4 key B, J5 pedal rest, J6 pedal down.',
        'J3 basket A, J4 basket B, J5 basket C, J6 water-change button.')
    destination = ROOT / name
    destination.parent.mkdir(parents=True, exist_ok=True)
    if not destination.exists():
        destination.write_text(text)
(ROOT / 'kicad').mkdir(exist_ok=True)
print('Created missing board-support and carrier generation sources')
