#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Reuse only jayis1's F401 acquisition/carrier platform from texture-cycle ffed391.
Refuses overwrites; does not import the preference experiment or its app model.
"""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
source=root.parent/'texture-cycle'
names=['firmware/main.c','firmware/board.h','firmware/registers.h',
       'firmware/startup.c','firmware/linker.ld','firmware/wire.h',
       'firmware/drivers/board.c','firmware/drivers/clock.c','firmware/drivers/serial.c',
       'firmware/Makefile','tests/test_clock.c','tests/test_serial.c',
       '.gitignore','LICENSE','tools/generate_hardware.py','tools/check_hardware.py',
       'app/transport.py']
for name in names:
    target=root/name
    if target.exists(): raise SystemExit('Refusing overwrite: '+str(target))
    text=(source/name).read_text()
    for old,new in [('texture-cycle','braid-trace'),('Texture Cycle','Braid Trace'),
                    ('TEXTURE_','BRAID_'),('TC_','BT_'),('TC1','BT1'),
                    ('captive swatch preference carrier','indexed crossing guide carrier'),
                    ('SELECT0','PAIR_LEFT'),('SELECT1','PAIR_RIGHT'),
                    ('REFERENCE','OVER'),('PREFER','UNDER'),
                    ('selector bit 0, selector bit 1','left detent, right detent')]:
        text=text.replace(old,new)
    target.parent.mkdir(parents=True,exist_ok=True)
    target.write_text(text)
print('Imported shared platform; author jayis1')
