#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Import only shared jayis1 carrier/acquisition infrastructure from 98f7d56.
The Float Band experiment engine, wire serializer, model and UI are independent.
"""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
source=root.parent/'label-veil'
names=['firmware/main.c','firmware/board.h','firmware/registers.h',
       'firmware/startup.c','firmware/linker.ld','firmware/drivers/board.c',
       'firmware/drivers/clock.c','firmware/drivers/serial.c','firmware/Makefile',
       'firmware/wire.h','tools/generate_hardware.py','tools/check_hardware.py',
       'app/transport.py']
for name in names:
    target=root/name
    if target.exists(): raise SystemExit('Refusing overwrite: '+str(target))
    text=(source/name).read_text()
    for old,new in [('label-veil','float-band'),('Label Veil','Float Band'),
                    ('LABEL_BOARD','FLOAT_BOARD'),('LABEL_WIRE','FLOAT_WIRE'),
                    ('LV1','FB1'),('demo.lv1','demo.fb1'),
                    ('tactile classification contact','reference-level contact'),
                    ('DOCK','LOW'),('CLASS_B','MID'),('ANSWER_A','HIGH'),('ANSWER_B','DUT')]:
        text=text.replace(old,new)
    target.parent.mkdir(parents=True,exist_ok=True)
    target.write_text(text)
print('Imported shared carrier/acquisition files; no experiment logic')
