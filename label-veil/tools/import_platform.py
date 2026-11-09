#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""One-time import of jayis1 acquisition/CAD infrastructure at 075829e.
No experiment engine, application model or experiment tests are imported.
"""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
source = root.parent / 'tooth-census'
names = ['firmware/main.c', 'firmware/board.h', 'firmware/registers.h',
         'firmware/startup.c', 'firmware/linker.ld', 'firmware/drivers/board.c',
         'firmware/drivers/clock.c', 'firmware/drivers/serial.c',
         'firmware/Makefile', 'firmware/wire.h', 'tests/test_clock.c',
         'tests/test_serial.c', '.gitignore', 'LICENSE',
         'tools/generate_hardware.py', 'tools/check_hardware.py', 'app/transport.py']
for name in names:
    destination = root / name
    if destination.exists():
        raise SystemExit('Refusing overwrite: ' + str(destination))
    text = (source / name).read_text()
    for old, new in [('tooth-census', 'label-veil'), ('Tooth Census', 'Label Veil'),
                     ('TOOTH_BOARD', 'LABEL_BOARD'), ('TOOTH_WIRE', 'LABEL_WIRE'),
                     ('TC1', 'LV1'), ('demo.tc1', 'demo.lv1'),
                     ('indexed tooth contact', 'tactile classification contact'),
                     ('ENC_A', 'DOCK'), ('ENC_B', 'CLASS_B'),
                     ('INDEX', 'ANSWER_A'), ('TOOTH', 'ANSWER_B')]:
        text = text.replace(old, new)
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(text)
print('Imported acquisition and CAD infrastructure only')
