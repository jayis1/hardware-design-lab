#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Import reviewed jayis1 platform from stroke-weave at 2d98d37.
Only board support, transport, CAD generation and driver tests are shared.
Refuse existing targets; this is provenance tooling, not a build dependency.
"""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
source = root.parent / 'stroke-weave'
names = ['firmware/main.c', 'firmware/board.h', 'firmware/registers.h',
         'firmware/startup.c', 'firmware/linker.ld', 'firmware/drivers/board.c',
         'firmware/drivers/clock.c', 'firmware/drivers/serial.c', 'firmware/Makefile',
         'firmware/wire.h', 'tests/test_clock.c', 'tests/test_serial.c', '.gitignore',
         'LICENSE', 'tools/generate_hardware.py', 'tools/check_hardware.py',
         'app/transport.py', 'tools/verify.py']
for name in names:
    target = root / name
    if target.exists():
        raise SystemExit('Refusing overwrite: ' + str(target))
    text = (source / name).read_text()
    for old, new in [('stroke-weave', 'age-rack'), ('Stroke Weave', 'Age Rack'),
                     ('STROKE_BOARD', 'AGE_BOARD'), ('STROKE_WIRE', 'AGE_WIRE'),
                     ('TW1', 'AR1'), ('demo.sw1', 'demo.ar1'),
                     ('twin-piston endpoint', 'sample-rack contact'),
                     ('A_HOME', 'RACK_A'), ('A_END', 'RACK_B'),
                     ('B_HOME', 'RACK_C'), ('B_END', 'RACK_D')]:
        text = text.replace(old, new)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text)
print('Imported shared platform; new rank engine and interface required')
