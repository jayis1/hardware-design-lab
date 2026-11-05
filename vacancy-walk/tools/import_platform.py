#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""One-time import of inspected jayis1 platform; never overwrite work."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
source = root.parent / 'toggle-hold'
names = ['firmware/main.c','firmware/board.h','firmware/registers.h',
         'firmware/startup.c','firmware/linker.ld','firmware/drivers/board.c',
         'firmware/drivers/clock.c','firmware/drivers/serial.c','firmware/Makefile',
         'firmware/wire.h','tests/test_clock.c','tests/test_serial.c','.gitignore',
         'LICENSE','tools/generate_hardware.py','tools/check_hardware.py','app/transport.py']
for name in names:
    destination = root / name
    if destination.exists():
        raise SystemExit('Refusing overwrite: '+str(destination))
    text = (source / name).read_text()
    for old,new in [('toggle-hold','vacancy-walk'),('Toggle Hold','Vacancy Walk'),
                    ('TOGGLE','VACANCY'),('TH1','VW1'),('demo.th1','demo.vw1'),
                    ('bistable retention contact','pocket occupancy contact'),
                    ('END_A','POCKET_0'),('END_B','POCKET_1'),
                    ('PUSH_B','POCKET_2'),('PUSH_A','POCKET_3')]:
        text=text.replace(old,new)
    destination.parent.mkdir(parents=True,exist_ok=True)
    destination.write_text(text)
print('Imported inspected platform by jayis1')
