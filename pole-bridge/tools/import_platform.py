#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Import shared jayis1 carrier/platform from age-rack at 54e1577.
New topology engine, protocol model and screens are designed separately.
Refuses existing files; not needed to build the committed self-contained device.
"""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
source=root.parent/'age-rack'
names=['firmware/main.c','firmware/board.h','firmware/registers.h',
       'firmware/startup.c','firmware/linker.ld','firmware/drivers/board.c',
       'firmware/drivers/clock.c','firmware/drivers/serial.c','firmware/Makefile',
       'firmware/wire.h','tests/test_clock.c','tests/test_serial.c','.gitignore',
       'LICENSE','tools/generate_hardware.py','tools/check_hardware.py',
       'app/transport.py','tools/verify.py']
for name in names:
    target=root/name
    if target.exists(): raise SystemExit('Refusing overwrite: '+str(target))
    text=(source/name).read_text()
    for old,new in [('age-rack','pole-bridge'),('Age Rack','Pole Bridge'),
                    ('AGE_BOARD','POLE_BOARD'),('AGE_WIRE','POLE_WIRE'),
                    ('AR1','PB1'),('demo.ar1','demo.pb1'),
                    ('sample-rack contact','two-pole contact'),
                    ('RACK_A','P1_A'),('RACK_B','P1_B'),('RACK_C','P2_A'),('RACK_D','P2_B')]:
        text=text.replace(old,new)
    target.parent.mkdir(parents=True,exist_ok=True)
    target.write_text(text)
print('Imported shared platform; no measurement engine imported')
