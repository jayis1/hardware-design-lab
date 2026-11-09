#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Import reviewed jayis1 platform from pole-bridge commit 80020a7.
No measurement engine or application model is copied. Refuse overwrites.
"""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
source=root.parent/'pole-bridge'
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
    for old,new in [('pole-bridge','sector-soak'),('Pole Bridge','Sector Soak'),
                    ('POLE_BOARD','SECTOR_BOARD'),('POLE_WIRE','SECTOR_WIRE'),
                    ('PB1,','SS1,'),(' PB1 ',' SS1 '),('demo.pb1','demo.ss1'),
                    ('two-pole contact','Gray-code contact'),
                    ('P1_A','GRAY0'),('P1_B','GRAY1'),('P2_A','GRAY2'),('P2_B','GRAY3')]:
        text=text.replace(old,new)
    target.parent.mkdir(parents=True,exist_ok=True)
    target.write_text(text)
print('Imported shared platform; measurement engine not imported')
